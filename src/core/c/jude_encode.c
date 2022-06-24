/*
 * The MIT License (MIT)
 * Copyright © 2022 James Parker
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <jude/jude_core.h>
#include <jude/core/c/jude_internal.h>

/**************************************
 * Declarations internal to this file *
 **************************************/
static bool checkreturn encode_field(jude_ostream_t *stream, const jude_field_t *field, const void *pData);
static bool checkreturn encode_array(jude_ostream_t *stream, const jude_field_t *field, const void *pData, jude_size_t count, jude_encoder_t *func);

static bool checkreturn jude_encode_tag_for_field(jude_ostream_t *stream, const jude_field_t *field)
{
   if (stream->suppress_first_tag)
   {
      stream->suppress_first_tag = false;
      return true;
   }

   return stream->transport->encode_tag(stream, field->type, field);
}

static jude_encoder_t *get_encoder(const jude_encode_transport_t *transport, jude_type_t type)
{
   switch (type)
   {
   case JUDE_TYPE_BOOL:     return transport->enc_bool;
   case JUDE_TYPE_SIGNED:   return transport->enc_signed;
   case JUDE_TYPE_UNSIGNED: return transport->enc_unsigned;
   case JUDE_TYPE_FLOAT:    return transport->enc_float;
   case JUDE_TYPE_ENUM:     return transport->enc_enum;
   case JUDE_TYPE_BITMASK:  return transport->enc_bitmask;
   case JUDE_TYPE_STRING:   return transport->enc_string;
   case JUDE_TYPE_BYTES:    return transport->enc_bytes;
   case JUDE_TYPE_OBJECT:   return transport->enc_object;
   default: 
      return NULL;
   }
}

/*************************
 * Encode a single field *
 *************************/

/* Encode a static array. Handles the size calculations and optional packing. */
static bool checkreturn encode_array(jude_ostream_t *stream, const jude_field_t *field, const void *pData, jude_size_t count, jude_encoder_t *func)
{
   jude_size_t i, noOfOutputMessages = 0;
   const void *p;
   bool skipSubMessage;

   if (count > field->array_size)
      return jude_ostream_error(stream, "array %s[%u] overflow", field->label, count);

   /* We always pack arrays if the datatype allows it. */
   if (stream->transport->is_packable(field))
   {
      ;
      if (!jude_encode_tag_for_field(stream, field))
      {
         return false;
      }

      if (!stream->transport->array_start(stream, field, pData, count, func))
      {
         return false;
      }

      /* Write the data */
      p = pData;

      for (i = 0; i < count; i++)
      {
         // should we skip submessage for lack of id?
         skipSubMessage = jude_field_is_object(field)
                      && !jude_filter_is_touched(((jude_object_t *) p)->__mask, JUDE_ID_FIELD_INDEX);

         if (!skipSubMessage)
         {
            if (!stream->transport->next_element(stream, noOfOutputMessages))
            {
               return false;
            }

            noOfOutputMessages++;

            if (!func(stream, field, p))
            {
               return false;
            }
         }

         p = (const char*) p + field->data_size;
      }

      if (!stream->transport->array_end(stream))
         return false;
   }
   else
   {
      if (count == 0)
         return true;

      p = pData;
      for (i = 0; i < count; i++)
      {
         if (!jude_encode_tag_for_field(stream, field))
            return false;

         /* Normally the data is stored directly in the array entries, but
          * for pointer-type string and bytes fields, the array entries are
          * actually pointers themselves also. So we have to dereference once
          * more to get to the actual data. */
         if (field->type == JUDE_TYPE_STRING || field->type == JUDE_TYPE_BYTES)
         {
            if (!func(stream, field, *(const void* const *) p))
               return false;
         }
         else
         {
            if (!func(stream, field, p))
               return false;
         }
         p = (const char*) p + field->data_size;
      }
   }

   return true;
}

/* Encode a field with static or pointer allocation, i.e. one whose data
 * is available to the encoder directly. */
static bool checkreturn encode_basic_field(jude_ostream_t *stream, const jude_field_t *field, const void *pData)
{
   jude_encoder_t *func = get_encoder(stream->transport, field->type);

   if (!jude_field_is_array(field))
   {
      if (!jude_encode_tag_for_field(stream, field))
         return false;

      if (!func(stream, field, pData))
         return false;
   }
   else
   {
      if (!encode_array(stream, field, pData, jude_get_array_count(field, pData), func))
         return false;
   }

   return true;
}

/* Encode a single field of any callback or static type. */
static bool checkreturn encode_null_field(jude_ostream_t *stream, const jude_field_t *field)
{
   if (!jude_encode_tag_for_field(stream, field))
      return false;

   return stream->transport->enc_null(stream, field, NULL);
}

/* Encode a single field of any callback or static type. */
static bool checkreturn encode_field(jude_ostream_t *stream, const jude_field_t *field, const void *pData)
{
   return encode_basic_field(stream, field, pData);
}

/*********************
 * Encode all fields *
 *********************/

static bool jude_is_field_to_be_encoded(const jude_filter_t *filter, jude_iterator_t *iter)
{
   // check if field is set or if its just been nulled and we want to output that "null" value...
   if (  jude_iterator_is_touched(iter)
      || jude_iterator_is_changed(iter)) // if field is "changed" but not "set" then we should output a JSON "null"
   {
      // check if a filter is in place...
      if (filter == NULL)
         return true;

      // check if this field is in the filter bitmask
      return jude_filter_is_touched(filter->mask, iter->field_index);
   }
   return false;
}


bool checkreturn jude_encode(jude_ostream_t *stream, const jude_object_t *src_struct)
{
   size_t field_count = 0;
   jude_filter_t filterMask = { {0} };

   if (stream->read_access_control != NULL)
   {
      stream->read_access_control(stream->read_access_control_ctx, src_struct, &filterMask);
   }
   else
   {
      jude_filter_fill_all(&filterMask);
   }

   if (!stream->transport->start_message(stream))
      return false;

   jude_iterator_t iter = jude_iterator_begin(jude_remove_const(src_struct));

   do
   {
      // Encode field if it is both set and in our filter mask
      if (jude_is_field_to_be_encoded(&filterMask, &iter))
      {
         // keep track of the member we are encoding for debugging
         stream->member = iter.current_field->label;
         stream->transport->next_element(stream, field_count);
         field_count++;

         if (jude_iterator_is_touched(&iter))
         {
            if (!encode_field(stream, iter.current_field, iter.details.data))
               return false;
         }
         else
         {
            if (!encode_null_field(stream, iter.current_field))
               return false;
         }
      }
   } while (jude_iterator_next(&iter));

   // Check if we have some extra output for our top level object...
   if (stream->extra_output_callback && src_struct->__parent_offset == 0)
   {
      if (!stream->transport->next_element(stream, field_count))
         return false;
      
      const char *name;
      const char *data;

      if (  !stream->extra_output_callback(stream->extra_output_callback_ctx, &name, &data)
         || !jude_ostream_write_json_tag(stream, name)
         || !jude_ostream_write_json_string(stream, data, strlen(data))
         )
      {
         return false;
      }
   }

   return stream->transport->end_message(stream);
}

bool jude_encode_delimited(jude_ostream_t *stream, const jude_object_t *src_struct)
{
   return stream->transport->enc_object(stream, src_struct->__rtti->field_list, src_struct);
}

bool jude_encode_single_value(jude_ostream_t *stream, const jude_iterator_t *iter)
{
   if (iter->current_field == NULL)
      return false;

   return encode_field(stream, iter->current_field, iter->details.sub_object);
}

bool jude_encode_single_element_of_array(jude_ostream_t *stream, const jude_iterator_t *iter, jude_size_t arrayIndex)
{
   const jude_field_t *field = iter->current_field;
   jude_encoder_t *func = get_encoder(stream->transport, field->type);

   if (field == NULL)
      return false;

   if (!jude_field_is_array(field))
   {
      return false;
   }

   if (arrayIndex >= jude_get_array_count(field, iter->details.data))
   {
      return false;
   }

   return func(stream, field, jude_get_array_data(field, iter->details.data, arrayIndex));
}
