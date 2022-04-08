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

/**************************************
 * Declarations internal to this file *
 **************************************/

static bool checkreturn decode_static_field(jude_istream_t *stream, jude_type_t wire_type, jude_iterator_t *iter);
static bool checkreturn decode_field(jude_istream_t *stream, jude_type_t wire_type, jude_iterator_t *iter);
static void jude_field_set_to_default(jude_iterator_t *iter);
static void jude_message_set_to_defaults(jude_object_t *dest_struct);
static bool checkreturn jude_decode_submessage(jude_istream_t *stream, const jude_field_t *field, void *dest);

/*************************
 * Decode a single field *
 *************************/
static jude_decoder_t *get_decoder(jude_istream_t* stream, jude_type_t type)
{
   if (!stream->transport)
   {
      // default transport
      stream->transport = jude_decode_transport_json;
   }

   const jude_decode_transport_t* transport = stream->transport;

   switch (type)
   {
   case JUDE_TYPE_BOOL:     return transport->dec_bool;
   case JUDE_TYPE_SIGNED:   return transport->dec_signed;
   case JUDE_TYPE_UNSIGNED: return transport->dec_unsigned;
   case JUDE_TYPE_FLOAT:    return transport->dec_float;
   case JUDE_TYPE_ENUM:     return transport->dec_enum;
   case JUDE_TYPE_BITMASK:  return transport->dec_bitmask;
   case JUDE_TYPE_STRING:   return transport->dec_string;
   case JUDE_TYPE_BYTES:    return transport->dec_bytes;
   case JUDE_TYPE_OBJECT:   return jude_decode_submessage;
   case JUDE_TYPE_NULL:     return transport->dec_string;
   }
   return NULL;
}

static bool jude_is_field_to_be_decoded(const jude_filter_t *filter, const jude_iterator_t *iter)
{
   // check if a filter is in place...
   if (filter == NULL)
      return true;

   // check if this field is in the filter bitmask
   return jude_filter_is_touched(filter->mask, iter->field_index);
}

static bool checkreturn decode_packed_array(jude_istream_t* stream, jude_iterator_t* iter)
{
   jude_decoder_t* func = get_decoder(stream, iter->current_field->type);

   bool status = true;
   jude_size_t* size = jude_iterator_get_count_reference(iter);
   jude_size_t original_size = *size;
   jude_istream_t substream;
   
   if (!stream->transport->context.open(JUDE_CONTEXT_REPEATED, stream, &substream))
      return false;

   /* A packed field should have it's size reset to zero on incoming message */
   if (!stream->always_append_repeated_fields)
   {
      *size = 0;
   }

   while (!stream->transport->context.is_eof(JUDE_CONTEXT_REPEATED, &substream))
   {
      if (*size >= iter->current_field->array_size)
      {
         return jude_istream_error(stream, "array overflow: %s", iter->current_field->label);
      }
      else
      {
         substream.field_got_nulled = false;
         void* pItem = (uint8_t*)iter->details.data + iter->current_field->data_size * (*size);
         if (!func(&substream, iter->current_field, pItem))
         {
            return false;
         }
         if (!substream.field_got_nulled)
         {
            if (jude_iterator_is_subresource(iter))
            {
               // If this was a new resource, let's generate a new id
               jude_object_t* new_object = jude_iterator_get_subresource(iter, *size);
               if (!jude_filter_is_touched(new_object->__mask, JUDE_ID_FIELD_INDEX))
               {
                  new_object->m_id = jude_generate_uuid();
                  jude_filter_set_touched(new_object->__mask, JUDE_ID_FIELD_INDEX, true);
               }
            }
            (*size)++; // only increment if the function produced a valid element
         }
         stream->transport->context.next_element(JUDE_CONTEXT_REPEATED, &substream);
      }
   }
   stream->transport->context.close(JUDE_CONTEXT_REPEATED, stream, &substream);

   if (substream.bytes_left != 0)
      return jude_istream_error(stream, "array overflow: %s", iter->current_field->label);

   // A change if substream detected a new value or we have a different final size
   stream->field_got_changed |= (substream.field_got_changed || original_size != *size);

   return status;
}

bool checkreturn decode_field_element(jude_istream_t* stream, jude_iterator_t* iter, jude_size_t index)
{
   jude_decoder_t* func = get_decoder(stream, iter->current_field->type);
   stream->member = iter->current_field->label;
   void* pItem = (uint8_t*)iter->details.data + iter->current_field->data_size * index;
   return func(stream, iter->current_field, pItem);
}

static bool checkreturn decode_static_field(jude_istream_t *stream, jude_type_t wire_type, jude_iterator_t *iter)
{
   if (!jude_iterator_is_array(iter))
   {
      // decode a single field (i.e. position 0)
      return decode_field_element(stream, iter, 0);
   }
   else if (stream->transport->is_packed(iter->current_field, wire_type))
   {
      /* Decoding all elements of a repeated field in one go... */
      return decode_packed_array(stream, iter);
   }
   else
   {
      /* Adding one more to a repeated field */
      jude_size_t* size = jude_iterator_get_count_reference(iter);
      if (*size >= iter->current_field->array_size)
      {
         return jude_istream_error(stream, "array overflow: %s", iter->current_field->label);
      }
      
      (*size)++;
      
      bool result = decode_field_element(stream, iter, (*size));
      if (!result || stream->field_got_nulled)
      {
         stream->field_got_nulled = false; // reset flag
         (*size)--;
      }
      return result;
   }
}

static bool checkreturn decode_field(jude_istream_t *stream, jude_type_t wire_type, jude_iterator_t *iter)
{
   // Assume the field got changed unless told otherwise:
   //stream->field_got_changed = true;
   stream->field_got_nulled = false;

   // for debugging, let's keep track of what member we are decoding...
   stream->member = iter->current_field->label;

   if (!decode_static_field(stream, wire_type, iter))
      return false;

   if (iter->current_field->always_notify)
   {
      stream->field_got_changed = true; // force "change flag"
   }

   if (stream->field_got_nulled)
   {
      jude_iterator_clear_touched(iter);
      stream->field_got_nulled = false; // reset flag
   }
   else
   {
      jude_iterator_set_touched(iter);
      if (stream->field_got_changed)
      {
         jude_iterator_set_changed(iter);
      }
   }

   return true;
}

/* Initialize message fields to default values, recursively */
static void jude_field_set_to_default(jude_iterator_t *iter)
{
   jude_type_t type = iter->current_field->type;

   bool init_data = true;
   if (!jude_iterator_is_array(iter))
   {
      /* Set has_field to false. Still initialize the optional field itself also. */
      jude_filter_set_touched(iter->object->__mask, iter->field_index, false);
   }
   else
   {
      /* REPEATED: Set array count to 0, no need to initialize contents.*/
      *(jude_size_t*)jude_iterator_get_count_reference(iter) = 0;
      init_data = false;
   }

   if (init_data)
   {
      if (jude_field_is_object(iter->current_field))
      {
         /* Initialize submessage to defaults */
         jude_message_set_to_defaults(iter->details.sub_object);
      }
      else if ((iter->current_field->details.default_data != NULL) && (type != JUDE_TYPE_ENUM))
      {
         /* Initialize to default value */
         memcpy(iter->details.data,
                iter->current_field->details.default_data,
                iter->current_field->data_size);
      }
      else
      {
         /* Initialize to zeros */
         memset(iter->details.data, 0, iter->current_field->data_size);
      }
   }
}

void jude_message_set_to_defaults(jude_object_t *dest_struct)
{
   jude_iterator_t iter = jude_iterator_begin(dest_struct);

   do
   {
      jude_field_set_to_default(&iter);
   } while (jude_iterator_next(&iter));
}

static bool checkreturn jude_decode_submessage(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   bool status;
   const jude_rtti_t* submsgType = field->details.sub_rtti;
   jude_object_t *dest_struct = (jude_object_t *) dest;
   jude_istream_t substream;

   if (submsgType == NULL || submsgType->field_list == NULL)
      return jude_istream_error(stream, "invalid field descriptor: %s", field->label);

   if (dest_struct->__rtti != submsgType)
      return jude_istream_error(stream,
            "destination sub message not initialised properly: %s",
            field->label);

   if (!stream->transport->context.open(JUDE_CONTEXT_SUBMESSAGE, stream,
         &substream))
      return false;

   /* New array entries need to be initialized, while required and optional
    * submessages have already been initialized in the top-level jude_decode. */
   if (jude_field_is_array(field))
   {
      status = jude_decode(&substream, dest_struct);
   }
   else
   {
      status = jude_decode_noinit(&substream, dest_struct);
   }

   if (status)
   {
      status = stream->transport->context.close(JUDE_CONTEXT_SUBMESSAGE, stream, &substream);
   }

   return status;

}

/*********************
 * Decode all fields *
 *********************/
static bool checkreturn jude_decode_noinit_interal(jude_istream_t *outer_stream, jude_object_t *dest_struct)
{
   bool first_field = true;
   jude_istream_t inner_stream;
   jude_istream_t *stream = &inner_stream;
   jude_filter_t filter = JUDE_EMPTY_FILTER;
   bool filterApplies = false;

   if (!outer_stream->transport) // if not given, use JSON by default...
   {
      outer_stream->transport = jude_decode_transport_json;
   }

   if (!outer_stream->transport->context.open(JUDE_CONTEXT_MESSAGE, outer_stream, &inner_stream))
   {
      return false;
   }

   jude_iterator_t iter = jude_iterator_begin(dest_struct);

   if (stream->write_access_control)
   {
      filterApplies = true;
      stream->write_access_control(stream->write_access_control_ctx, dest_struct, &filter);
   }

   while (!stream->transport->context.is_eof(JUDE_CONTEXT_MESSAGE, stream))
   {
      uint32_t tag = JUDE_TAG_UNKNOWN;
      jude_type_t wire_type;
      bool eof = false;

      if (!first_field)
      {
         /* Move to next element if transport requires */
         if (!stream->transport->context.next_element(JUDE_CONTEXT_MESSAGE, stream))
            return false;
      }
      else
      {
         first_field = false;
      }

      if (!stream->transport->decode_tag(stream, dest_struct, &wire_type, &tag, &eof))
      {
         if (eof)
            break;
         else
            return false;
      }

      if (tag == JUDE_TAG_UNKNOWN_BUT_HANDLED)
      {
         continue;
      }

      // reset our iterator and continue
      jude_iterator_reset(&iter);

      if (  !jude_iterator_find(&iter, tag)
         || (filterApplies && !jude_is_field_to_be_decoded(&filter, &iter)))
      {
         /* No match found, skip data */
         if (!stream->transport->skip_field(stream, wire_type))
            return false;

         continue;
      }

      if (!decode_field(stream, wire_type, &iter))
         return false;
   }

   // Has an error occured?
   if (stream->has_error)
      return false;

   if (!outer_stream->transport->context.close(JUDE_CONTEXT_MESSAGE, outer_stream, &inner_stream))
   {
      return false;
   }

   return true;
}

bool checkreturn jude_decode_noinit(jude_istream_t *stream, jude_object_t *dest_struct)
{
   if (!jude_decode_noinit_interal(stream, dest_struct))
   {
      stream->has_error = true;
      return false;
   }
   return true;
}

bool jude_decode_single_field(jude_istream_t *stream, jude_iterator_t *iter)
{
   if (iter->current_field == NULL)
      return false;

   jude_type_t unusedWireType = JUDE_TYPE_UNSIGNED;

   return decode_field(stream, unusedWireType, iter);
}

bool checkreturn jude_decode(jude_istream_t *stream, jude_object_t *dest_struct)
{
   bool status;
   jude_message_set_to_defaults(dest_struct);
   status = jude_decode_noinit(stream, dest_struct);

   return status;
}

bool jude_decode_delimited(jude_istream_t *stream, jude_object_t *dest_struct)
{
   jude_istream_t substream;
   bool status;

   if (!stream->transport->context.open(JUDE_CONTEXT_DELIMITED, stream, &substream))
      return false;

   status = jude_decode(&substream, dest_struct);
   stream->transport->context.close(JUDE_CONTEXT_DELIMITED, stream, &substream);
   return status;
}
