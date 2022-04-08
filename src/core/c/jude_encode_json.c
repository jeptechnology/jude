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

#include <inttypes.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <jude/jude_core.h>
#include <jude/core/c/jude_internal.h>


/*
 * To prevent warnings:
 */
static bool json_write(jude_ostream_t *stream, const char *buf, size_t count)
{
   return jude_ostream_write(stream, (const uint8_t *) buf, count);
}

bool checkreturn jude_ostream_write_json_tag(jude_ostream_t *stream, const char *tag)
{
   return   json_write(stream, "\"", 1)
         && json_write(stream, tag, strlen(tag))
         && json_write(stream, "\":", 2);
}

static bool checkreturn json_encode_tag(jude_ostream_t *stream, jude_type_t wiretype, const jude_field_t *field)
{
   return jude_ostream_write_json_tag(stream, field->label);
}

bool checkreturn jude_ostream_write_json_string(jude_ostream_t *stream, const char *buffer, size_t size)
{
   /* catch a null string here */
   if (buffer == NULL /*|| buffer[0] == '\0'*/)
   {
      return json_write(stream, "null", 4);
   }

   if (!json_write(stream, "\"", 1))
   {
      return false;
   }
   
   bool escaped_flag = false;
   const char *ptr = buffer;
   while (ptr < (buffer + size) && *ptr != '\0')
   {
      if (*ptr == '\\' || *ptr == '"')
      {
         escaped_flag = true;
      }
      ptr++;
   }

   /* If required, clip size to our string length */
   if (ptr < buffer + size)
   {
      size = ptr - buffer;
   }

   if (!escaped_flag)
   {
      if (size > 0 && !json_write(stream, buffer, size))
      {
         return false;
      }
   }
   else
   {
      /* need to be careful to escape the quotes */
      const char *ptr = buffer;
      while (ptr < (buffer + size))
      {
         if (*ptr == '\\' || *ptr == '"')
         {
            if (!json_write(stream, "\\", 1))
               return false;
         }
         if (!json_write(stream, ptr, 1))
            return false;
         ptr++;
      }
   }

   return json_write(stream, "\"", 1);
}

static size_t json_print_signed_number(char *buffer, size_t bufferLength, const jude_field_t *field, const void *src)
{
   if (field->data_size < 8)
   {
      int32_t svalue32 = 0;
      switch (field->data_size)
      {
      case 1:
         svalue32 = *(const int8_t*) src;
         break;
      case 2:
         svalue32 = *(const int16_t*) src;
         break;
      case 4:
         svalue32 = *(const int32_t*) src;
         break;
      }
      return snprintf(buffer, bufferLength, "%" PRId32, svalue32);
   }
   else if (field->data_size == 8)
   {
      return snprintf(buffer, bufferLength, "%" PRId64, *(const int64_t*) src);
   }

   return 0;
}

static size_t printToBuffer(char* buffer, size_t bufferLength, uint64_t value)
{
   size_t const base = 10;
   char* ptr = buffer, *ptr1 = buffer, tmp_char;
   size_t charsWritten = 0;
   uint64_t tmp_value;

   do
   {
      tmp_value = value;
      value /= base;
      *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
   } while (value);

   charsWritten = ptr - buffer;
   *ptr-- = '\0';
   while (ptr1 < ptr)
   {
      tmp_char = *ptr;
      *ptr-- = *ptr1;
      *ptr1++ = tmp_char;
   }
   return charsWritten;
}

static size_t json_print_unsigned_number(char *buffer, size_t bufferLength, const jude_field_t *field, const void *src)
{
   if (field->data_size < 8)
   {
      uint32_t value32 = 0;
      switch (field->data_size)
      {
      case 1:
         value32 = *(const uint8_t*) src;
         break;
      case 2:
         value32 = *(const uint16_t*) src;
         break;
      case 4:
         value32 = *(const uint32_t*) src;
         break;
      }
      return snprintf(buffer, bufferLength, "%" PRIu32, value32);
   }
   else if (field->data_size == 8)
   {
      return printToBuffer(buffer, bufferLength, *(const uint64_t*) src);
   }

   return 0;
}

static bool json_enc_enum(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   uint32_t value32 = 0;
   switch (field->data_size)
   {
   case 1:
      value32 = *(const uint8_t*) src;
      break;
   case 2:
      value32 = *(const uint16_t*) src;
      break;
   case 4:
      value32 = *(const uint32_t*) src;
      break;
   }

   if (field->details.enum_map)
   {
      // decode the string for this enum and find it
      const jude_enum_map_t *enum_map = field->details.enum_map;

      if (field->type == JUDE_TYPE_BITMASK)
      {
         bool commaRequired = false;

         // Special case: We need to run through each bit in the mask and output a JSON string array
         // containing each bit that is "true"
         if (!json_write(stream, "[", 1))
            return false;

         while (enum_map->name)
         {
            if (jude_bitfield_is_set((jude_bitfield_t)&value32, enum_map->value))
            {
               if (commaRequired && !json_write(stream, ",", 1))
                  return false;
               commaRequired = true;

               if (!jude_ostream_write_json_string(stream, enum_map->name, strlen(enum_map->name)))
                  return false;
            }

            enum_map++;
         }

         return json_write(stream, "]", 1);
      }
      else
      {
         const char *enum_string = jude_enum_find_string(enum_map, value32);
         if (enum_string)
            return jude_ostream_write_json_string(stream, enum_string,  strlen(enum_string));
         else
            return jude_ostream_error(stream, "enum value '%lu' not valid", value32, field->label);
      }
   }

   return jude_ostream_error(stream, "enum field has no enum map");
}

/* Field encoders */
bool checkreturn json_enc_number(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   char buffer[32];
   size_t len;
   double d;

   switch (field->type)
   {
   case JUDE_TYPE_BOOL:
      len = snprintf(buffer, sizeof(buffer),
            (*(const bool*) src) ? "true" : "false");
      break;

   case JUDE_TYPE_FLOAT:
      switch (field->data_size)
      {
      case 4:
         len = snprintf(buffer, sizeof(buffer), "%g", *(const float*) src);
         break;
      case 8:
         d = *(const double*) src;
         if (fabs(floor(d) - d) <= DBL_EPSILON)
            len = snprintf(buffer, sizeof(buffer), "%.0g", d);
         else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9)
            len = snprintf(buffer, sizeof(buffer), "%e", d);
         else
            len = snprintf(buffer, sizeof(buffer), "%g", d);
         break;
      default:
         return false;
      }
      break;

   case JUDE_TYPE_SIGNED:
      len = json_print_signed_number(buffer, sizeof(buffer), field, src);
      break;

   case JUDE_TYPE_ENUM:
   case JUDE_TYPE_BITMASK:
      return json_enc_enum(stream, field, src);
      break;

   default:
      len = json_print_unsigned_number(buffer, sizeof(buffer), field, src);
      break;
   }

   return json_write(stream, buffer, len);
}

bool checkreturn json_enc_null(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   return json_write(stream, "null", 4);
}

bool checkreturn json_enc_bytes(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   if (src == NULL)
   {
      return json_write(stream, "null", 4);
   }
   else
   {
      const jude_bytes_array_t *bytes = (const jude_bytes_array_t*) src;

      if (bytes->size + offsetof(jude_bytes_array_t, bytes) > field->data_size)
         return jude_ostream_error(stream, "bytes size exceeded");

      if (!json_write(stream, "\"", 1)
            || !json_base64_write(stream, bytes->bytes, bytes->size)
            || !json_write(stream, "\"", 1))
      {
         return false;
      }
   }
   return true;
}

bool checkreturn json_enc_string(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   size_t max_len = field->data_size;

   return jude_ostream_write_json_string(stream, (const char*) src, max_len);
}

static bool json_is_packable_field(const jude_field_t *field)
{
   // all fields in JSON wire format are packable!
   return true;
}

static bool json_array_start(jude_ostream_t *stream, const jude_field_t *field, const void *pData, size_t count, jude_encoder_t *func)
{
   return json_write(stream, "[", 1);
}

static bool json_array_end(jude_ostream_t *stream)
{
   return json_write(stream, "]", 1);
}

static bool json_start_message(jude_ostream_t *stream)
{
   return json_write(stream, "{", 1);
}

static bool json_end_message(jude_ostream_t *stream)
{
   return json_write(stream, "}", 1);
}

static bool json_next_element(jude_ostream_t *stream, size_t index)
{
   if (index > 0)
      return json_write(stream, ",", 1);

   return true;
}

static bool checkreturn json_enc_submessage(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   if (field->details.sub_rtti == NULL)
   {
      return jude_ostream_error(stream, "invalid field descriptor");
   }

   if (field->details.sub_rtti != ((jude_object_t *) src)->__rtti)
   {
      return jude_ostream_error(stream, "Sub message type info not initialised");
   }

   return jude_encode(stream, (jude_object_t *) src);
}

static const jude_encode_transport_t transport =
{
   .enc_bool     = json_enc_number,
   .enc_signed   = json_enc_number,
   .enc_unsigned = json_enc_number,
   .enc_float    = json_enc_number,
   .enc_enum     = json_enc_number,
   .enc_bitmask  = json_enc_number,
   .enc_string   = json_enc_string,
   .enc_bytes    = json_enc_bytes,
   .enc_object   = json_enc_submessage,
   .enc_null     = json_enc_null,

   .encode_tag  = json_encode_tag,
   .is_packable = json_is_packable_field,

   .start_message = json_start_message,
   .end_message   = json_end_message,

   .array_start = json_array_start,
   .array_end   = json_array_end,

   .next_element = json_next_element
};

const jude_encode_transport_t *jude_encode_transport_json = &transport;
