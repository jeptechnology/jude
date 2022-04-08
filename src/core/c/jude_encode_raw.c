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

#include <inttypes.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>

static bool checkreturn raw_encode_tag(jude_ostream_t *stream, jude_type_t wiretype, const jude_field_t *field)
{
   return true;
}

/* Field encoders */
bool checkreturn raw_enc_number(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   uint8_t bitWidth = (uint8_t) field->data_size;
   uint8_t outputWidth = 0;
   uint64_t value64;
   uint32_t value32;
   uint16_t value16;
   uint8_t value8;
   const uint8_t* valueData;

   outputWidth = bitWidth;

   switch (bitWidth)
   {
   case sizeof(uint8_t):
      value64 = *(const uint8_t*) src;
      break;
   case sizeof(uint16_t):
      value64 = *(const uint16_t*) src;
      break;
   case sizeof(uint32_t):
      value64 = *(const uint32_t*) src;
      break;
   case sizeof(uint64_t):
      value64 = *(const uint64_t*) src;
      break;
   default:
      return jude_ostream_error(stream, "Unexpected numeric width: %d", bitWidth);
   }

   switch (outputWidth)
   {
   case sizeof(uint8_t):
      value8 = (uint8_t) value64;
      valueData = (uint8_t *) &value8;
      break;
   case sizeof(uint16_t):
      value16 = (uint16_t) value64;
      valueData = (uint8_t *) &value16;
      break;
   case sizeof(uint32_t):
      value32 = (uint32_t) value64;
      valueData = (uint8_t *) &value32;
      break;
   case sizeof(uint64_t):
      valueData = (uint8_t *) &value64;
      break;
   default:
      return jude_ostream_error(stream, "Unexpected numeric width: %d", bitWidth);
   }

   return jude_ostream_write(stream, valueData, outputWidth);
}

bool checkreturn raw_enc_bytes(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   const jude_bytes_array_t *bytes = (const jude_bytes_array_t*) src;

   if (src == NULL)
      return true;

   return jude_ostream_write(stream, bytes->bytes, bytes->size);
}

bool checkreturn raw_enc_string(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   size_t length = strlen((const char*) src);
   uint8_t terminator = '\0';

   if (  !jude_ostream_write(stream, (const uint8_t*) src, length)
      || !jude_ostream_write(stream, &terminator, 1))
   {
      return false;
   }

   return true;
}

static bool raw_is_packable_field(const jude_field_t *field)
{
   return true;
}

static bool raw_array_start(jude_ostream_t *stream, const jude_field_t *field,
      const void *pData, size_t count, jude_encoder_t *func)
{
   return true;
}

static bool raw_array_end(jude_ostream_t *stream)
{
   return true;
}

static bool raw_start_message(jude_ostream_t *stream)
{
   return true;
}

static bool raw_end_message(jude_ostream_t *stream)
{
   return true;
}

static bool raw_next_element(jude_ostream_t *stream, size_t index)
{
   return true;
}

static bool checkreturn raw_enc_submessage(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   return jude_ostream_error(stream, "raw does not support submessages");
}

static const jude_encode_transport_t transport =
{
   .enc_bool = raw_enc_number,
   .enc_signed = raw_enc_number,
   .enc_unsigned = raw_enc_number,
   .enc_float = raw_enc_number,
   .enc_enum = raw_enc_number,
   .enc_bitmask = raw_enc_number,
   .enc_string = raw_enc_string,
   .enc_bytes = raw_enc_bytes,
   .enc_object = raw_enc_submessage,

   .encode_tag = raw_encode_tag, 
   .is_packable = raw_is_packable_field,

   .start_message = raw_start_message,
   .end_message = raw_end_message,

   .array_start = raw_array_start,
   .array_end = raw_array_end,

   .next_element = raw_next_element
};

const jude_encode_transport_t *jude_encode_transport_raw = &transport;
