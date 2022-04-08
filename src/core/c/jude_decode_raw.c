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

#include <string.h>
#include <math.h>
#include <stdbool.h>

typedef uint64_t raw_number_t;

/*
 * Linked list structure for storing allocated
 * entries during an array decode of a pointer
 */
typedef struct repeat_array_s
{
   void *ptr;
   struct repeat_array_s *next;
} repeat_array_t;

#define READ_NEXT(stream, ch) \
   if (!jude_istream_read((stream), ((uint8_t*)&(ch)), 1)) return jude_istream_error(stream, "Unexpected EOF")

/********************
 * Helper functions *
 ********************/
static bool checkreturn
raw_read_string(jude_istream_t *stream, char *buf, jude_size_t max_count,
      jude_size_t *length, bool terminate)
{
   jude_size_t bytes_read = 0;

   // sanitize input!
   if (max_count < 1)
   {
      return false;
   }

   *length = (jude_size_t) stream->bytes_left;

   if (*length > max_count)
      return jude_istream_error(stream, "string overflow: requested=%d, max=%d", *length,
            max_count);

   stream->field_got_changed = false;

   while (bytes_read < *length)
   {
      char c;
      READ_NEXT(stream, c);
      if (c != buf[bytes_read])
      {
         stream->field_got_changed = true;
      }
      buf[bytes_read] = c;
      bytes_read++;
   }

   // always terminate string
   if (terminate)
   {
      if (bytes_read < max_count)
      {
         buf[bytes_read] = '\0';
      }
      else
      {
         buf[max_count - 1] = '\0';
      }
   }
   stream->bytes_left = 0; // Raw streams only contain a single value, so now that we've read it we know we're at the end of the stream.
   return true;
}

static bool raw_decode_tag(jude_istream_t *stream, jude_object_t *object, jude_type_t *wire_type, uint32_t *tag, bool *eof)
{
   uint8_t loByte = 0;
   uint8_t hiByte = 0;

   if (!jude_istream_read(stream, (uint8_t*) &hiByte, 1)
         || !jude_istream_read(stream, (uint8_t*) &loByte, 1))
   {
      *eof = true;
      return false;
   }
   else
   {
      stream->last_char = loByte;
      *tag = (((uint32_t) hiByte) << 8) + loByte;
   }

   return true;
}

/* Parse the input text to generate a number, and populate the result into item.
 * last_char will be populated with the last char to be read
 */
static bool checkreturn
raw_read_number(jude_istream_t *stream, raw_number_t *number,
      jude_size_t expectedWidth)
{
   switch (expectedWidth)
   {
   case sizeof(uint8_t):
   {
      uint8_t data;
      jude_istream_read(stream, &data, sizeof(data));
      *number = data;
      break;
   }
   case sizeof(uint16_t):
   {
      uint16_t data;
      jude_istream_read(stream, (uint8_t*) &data, sizeof(data));
      *number = data;
      break;
   }
   case sizeof(uint32_t):
   {
      uint32_t data;
      jude_istream_read(stream, (uint8_t*) &data, sizeof(data));
      *number = data;
      break;
   }
   case sizeof(uint64_t):
   {
      uint64_t data;
      jude_istream_read(stream, (uint8_t*) &data, sizeof(data));
      *number = data;
      break;
   }
   default:
      return jude_istream_error(stream, "unexpected numeric length: %d", expectedWidth);
   }

   stream->bytes_left = 0; // Raw streams only contain a single value, so now that we've read it we know we're at the end of the stream.
   return true;
}

static bool checkreturn
raw_skip_field(jude_istream_t *stream, jude_type_t wire_type)
{
   stream->bytes_left = 0; // Raw streams only contain a single value, so now that we've skipped it we know we're at the end of the stream.
   return true;
}

/*
 * Note to reader:
 *
 * This ASSIGN macro is painful to read but essentially it does two things
 * which in the absence of template code (thank you ANSI C) we cannot achieve
 * easily with a single function:
 *
 * 1. determines if the destination and source are equal and "returns" this
 * 2. makes the assignment "dest = source"
 *
 * We need to do this for all 4 bit widths of both signed and unsigned ints (that's 8 in all)
 */
#define ASSIGN(type, dest, source) \
   *(type*)(dest) != (type)(source); *(type*)(dest) = (type)(source)

static bool raw_apply_number_value(jude_istream_t *stream,
      const jude_field_t *field, void *dest, raw_number_t number)
{
   bool haschanged = true;

   switch (field->type)
   {
   case JUDE_TYPE_FLOAT:
      return jude_istream_error(stream, "raw floats not supported");
      break;

   case JUDE_TYPE_SIGNED:
      switch (field->data_size)
      {
      case 1:
         haschanged = ASSIGN(int8_t, dest, number)
         ;
         break;
      case 2:
         haschanged = ASSIGN(int16_t, dest, number)
         ;
         break;
      case 4:
         haschanged = ASSIGN(int32_t, dest, number)
         ;
         break;
      case 8:
         haschanged = ASSIGN(int64_t, dest, number)
         ;
         break;
      default:
         return jude_istream_error(stream, "invalid data_size: %d", field->data_size);
      }
      break;

   default:
      switch (field->data_size)
      {
      case 1:
         haschanged = ASSIGN(uint8_t, dest, number)
         ;
         break;
      case 2:
         haschanged = ASSIGN(uint16_t, dest, number)
         ;
         break;
      case 4:
         haschanged = ASSIGN(uint32_t, dest, number)
         ;
         break;
      case 8:
         haschanged = ASSIGN(uint64_t, dest, number)
         ;
         break;
      default:
         return jude_istream_error(stream, "invalid data_size: %d", field->data_size);
      }
      break;
   }

   stream->field_got_changed = haschanged;

   return true;
}

/* Field decoders */
static bool checkreturn
raw_dec_number(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   raw_number_t number = 0;
   jude_size_t bytesToRead = (jude_size_t) stream->bytes_left;
   bool overflowCheckRequired = true;

   bytesToRead = field->data_size;
   if (bytesToRead < field->data_size)
   {
      return jude_istream_error(stream,
            "Field's data size is greater than number of bytes left to read");
   }

   if (!raw_read_number(stream, &number, bytesToRead))
      return false;

   if (overflowCheckRequired)
   {
      bool overflow = false;

      switch (field->data_size)
      {
      case 1:
         overflow = number > UINT8_MAX;
         break;
      case 2:
         overflow = number > UINT16_MAX;
         break;
      case 4:
         overflow = number > UINT32_MAX;
         break;
      }

      if (overflow)
      {
         return jude_istream_error(stream, "Expected width: %d, Actual width: %d",
               field->data_size, stream->bytes_left);
      }
   }

   return raw_apply_number_value(stream, field, dest, number);
}

static bool checkreturn
raw_dec_bytes(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   jude_bytes_array_t *bytesArray = (jude_bytes_array_t*) dest;
   jude_size_t max_length = field->data_size - offsetof(jude_bytes_array_t, bytes);

   // Reading a string will read the next byte as a length field and the rest as the contents
   if (!raw_read_string(stream, (char *) bytesArray->bytes, max_length,
         &bytesArray->size, false))
      return false;

   /* Check length, noting the space taken by the jude_size_t object. */
   if (bytesArray->size > max_length)
      return jude_istream_error(stream, "bytes overflow");

   stream->last_char = '"';
   stream->bytes_left = 0; // Raw streams only contain a single value, so now that we've read it we know we're at the end of the stream.
   return true;
}

static bool checkreturn
raw_dec_string(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   jude_size_t length;
   return raw_read_string(stream, (char*) dest, field->data_size, &length,
   true);
}

static bool raw_context_substream_open(jude_context_type_t type,
      jude_istream_t *stream, jude_istream_t *substream)
{
   jude_buffer_transfer(&substream->buffer, &stream->buffer);
   return true;
}

static bool raw_context_substream_is_eof(jude_context_type_t type,
      jude_istream_t *stream)
{
   return stream->bytes_left == 0;
}

static bool raw_context_substream_next_element(jude_context_type_t type,
      jude_istream_t *substream)
{
   return substream->bytes_left > 0;
}

static bool raw_context_substream_close(jude_context_type_t type,
      jude_istream_t *stream, jude_istream_t *substream)
{
   memcpy(stream, substream, sizeof(jude_istream_t));
   return true;
}

static bool raw_is_packed(const jude_field_t *field, jude_type_t wire_type)
{
   return true;
}

/* --- JSON decoding transport layer --- */
static const jude_decode_transport_t transport =
{
   .dec_bool     = &raw_dec_number,
   .dec_signed   = &raw_dec_number,
   .dec_unsigned = &raw_dec_number,
   .dec_float    = &raw_dec_number,
   .dec_enum     = &raw_dec_number,
   .dec_bitmask  = &raw_dec_number,
   .dec_bytes    = &raw_dec_bytes,
   .dec_string   = &raw_dec_string,

   .decode_tag = &raw_decode_tag,
   .is_packed = &raw_is_packed,
   .skip_field = &raw_skip_field,

   .context.open = raw_context_substream_open,
   .context.is_eof = raw_context_substream_is_eof,
   .context.next_element = raw_context_substream_next_element,
   .context.close = raw_context_substream_close
};

const jude_decode_transport_t *jude_decode_transport_raw = &transport;
