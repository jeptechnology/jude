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

static bool checkreturn protobuf_dec_varint32(jude_istream_t *stream, uint32_t *dest)
{
   uint8_t byte;
   uint32_t result;

   if (!jude_istream_readbyte(stream, &byte))
      return false;

   if ((byte & 0x80) == 0)
   {
      /* Quick case, 1 byte value */
      result = byte;
   }
   else
   {
      /* Multibyte case */
      uint8_t bitpos = 7;
      result = byte & 0x7F;

      do
      {
         if (bitpos >= 32)
            return jude_istream_error(stream, "varint overflow");

         if (!jude_istream_readbyte(stream, &byte))
            return false;

         result |= (uint32_t) (byte & 0x7F) << bitpos;
         bitpos = (uint8_t) (bitpos + 7);
      } while (byte & 0x80);
   }

   *dest = result;
   return true;
}

static bool checkreturn protobuf_dec_varint64(jude_istream_t *stream, uint64_t *dest)
{
   uint8_t byte;
   uint8_t bitpos = 0;
   uint64_t result = 0;

   do
   {
      if (bitpos >= 64)
         return jude_istream_error(stream, "varint overflow");

      if (!jude_istream_readbyte(stream, &byte))
         return false;

      result |= (uint64_t) (byte & 0x7F) << bitpos;
      bitpos = (uint8_t) (bitpos + 7);
   } while (byte & 0x80);

   *dest = result;
   return true;
}

static bool checkreturn jude_skip_varint(jude_istream_t *stream)
{
   uint8_t byte;
   do
   {
      if (!jude_istream_read(stream, &byte, 1))
         return false;
   } while (byte & 0x80);
   return true;
}

static bool checkreturn jude_skip_string(jude_istream_t *stream)
{
   uint32_t length;
   if (!protobuf_dec_varint32(stream, &length))
      return false;

   return jude_istream_read(stream, NULL, length);
}

static bool checkreturn protobuf_dec_tag(jude_istream_t *stream, jude_object_t *object, jude_type_t *wire_type, uint32_t *tag, bool *eof)
{
   uint32_t temp;
   *eof = false;
   *wire_type = (jude_type_t) 0;
   *tag = 0;

   if (!protobuf_dec_varint32(stream, &temp))
   {
      if (stream->bytes_left == 0)
         *eof = true;

      return false;
   }

   if (temp == 0)
   {
      *eof = true; /* Special feature: allow 0-terminated messages. */
      return false;
   }

   *tag = temp >> 3;
   *wire_type = (jude_type_t) (temp & 7);
   return true;
}

static bool checkreturn protobuf_skip_field(jude_istream_t *stream, jude_type_t wire_type)
{
   switch (get_protobuf_wire_type(wire_type))
   {
   case PROTOBUF_WT_VARINT:
      return jude_skip_varint(stream);
   case PROTOBUF_WT_STRING:
      return jude_skip_string(stream);
   default:
      return jude_istream_error(stream, "invalid wire_type");
   }
}

/* Read a raw value to buffer, for the purpose of passing it to callback as
 * a substream. Size is maximum size on call, and actual size on return.
 */
static bool checkreturn read_raw_value(jude_istream_t *stream, jude_type_t wire_type, uint8_t *buf,
      size_t *size)
{
   size_t max_size = *size;
   switch (get_protobuf_wire_type(wire_type))
   {
   case PROTOBUF_WT_VARINT:
      *size = 0;
      do
      {
         (*size)++;
         if (*size > max_size)
            return false;
         if (!jude_istream_read(stream, buf, 1))
            return false;
      } while (*buf++ & 0x80);
      return true;

   default:
      return jude_istream_error(stream, "invalid wire_type");
   }
}

/* Field decoders */

static bool protobuf_dec_svarint64(jude_istream_t *stream, int64_t *dest)
{
   uint64_t value;
   if (!protobuf_dec_varint64(stream, &value))
      return false;

   if (value & 1)
      *dest = (int64_t) (~(value >> 1));
   else
      *dest = (int64_t) (value >> 1);

   return true;
}

static bool protobuf_dec_fixed32(jude_istream_t *stream, void *dest)
{
#ifdef __BIG_ENDIAN__
   uint8_t *bytes = (uint8_t*)dest;
   uint8_t lebytes[4];

   if (!jude_istream_read(stream, lebytes, 4))
   return false;

   bytes[0] = lebytes[3];
   bytes[1] = lebytes[2];
   bytes[2] = lebytes[1];
   bytes[3] = lebytes[0];
   return true;
#else
   return jude_istream_read(stream, (uint8_t*) dest, 4);
#endif
}

static bool protobuf_dec_fixed64(jude_istream_t *stream, void *dest)
{
#ifdef __BIG_ENDIAN__
   uint8_t *bytes = (uint8_t*)dest;
   uint8_t lebytes[8];

   if (!jude_istream_read(stream, lebytes, 8))
   return false;

   bytes[0] = lebytes[7];
   bytes[1] = lebytes[6];
   bytes[2] = lebytes[5];
   bytes[3] = lebytes[4];
   bytes[4] = lebytes[3];
   bytes[5] = lebytes[2];
   bytes[6] = lebytes[1];
   bytes[7] = lebytes[0];
   return true;
#else
   return jude_istream_read(stream, (uint8_t*) dest, 8);
#endif
}

static bool checkreturn protobuf_dec_varint(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   uint64_t value;
   int64_t svalue;
   int64_t clamped;
   if (!protobuf_dec_varint64(stream, &value))
      return false;

   /* See issue 97: Google's C++ protobuf allows negative varint values to
    * be cast as int32_t, instead of the int64_t that should be used when
    * encoding. Previous jude versions had a bug in encoding. In order to
    * not break decoding of such messages, we cast <=32 bit fields to
    * int32_t first to get the sign correct.
    */
   if (field->data_size == 8)
      svalue = (int64_t) value;
   else
      svalue = (int32_t) value;

   switch (field->data_size)
   {
   case 1:
      clamped = *(int8_t*) dest = (int8_t) svalue;
      break;
   case 2:
      clamped = *(int16_t*) dest = (int16_t) svalue;
      break;
   case 4:
      clamped = *(int32_t*) dest = (int32_t) svalue;
      break;
   case 8:
      clamped = *(int64_t*) dest = svalue;
      break;
   default:
      return jude_istream_error(stream, "invalid data_size: %s", field->label);
   }

   if (clamped != svalue)
      return jude_istream_error(stream, "integer too large: %s", field->label);

   return true;
}

static bool checkreturn
protobuf_dec_uvarint(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   uint64_t value, clamped;
   if (!protobuf_dec_varint64(stream, &value))
      return false;

   switch (field->data_size)
   {
   case 1:
      clamped = *(uint8_t*) dest = (uint8_t) value;
      break;
   case 2:
      clamped = *(uint16_t*) dest = (uint16_t) value;
      break;
   case 4:
      clamped = *(uint32_t*) dest = (uint32_t) value;
      break;
   case 8:
      clamped = *(uint64_t*) dest = value;
      break;
   default:
      return jude_istream_error(stream, "invalid data_size: %s", field->label);
   }

   if (clamped != value)
      return jude_istream_error(stream, "integer too large: %s", field->label);

   return true;
}

static bool checkreturn
protobuf_dec_svarint(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   int64_t value = 0, clamped = 0;
   if (!protobuf_dec_svarint64(stream, &value))
      return false;

   switch (field->data_size)
   {
   case 1:
      clamped = *(int8_t*) dest = (int8_t) value;
      break;
   case 2:
      clamped = *(int16_t*) dest = (int16_t) value;
      break;
   case 4:
      clamped = *(int32_t*) dest = (int32_t) value;
      break;
   case 8:
      clamped = *(int64_t*) dest = value;
      break;
   default:
      return jude_istream_error(stream, "invalid data_size: %s", field->label);
   }

   if (clamped != value)
      return jude_istream_error(stream, "integer too large: %s", field->label);

   return true;
}

static bool checkreturn protobuf_dec_bytes(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   uint32_t size;
   size_t alloc_size;
   jude_bytes_array_t *bdest;

   if (!protobuf_dec_varint32(stream, &size))
      return false;

   if (size > JUDE_SIZE_MAX)
      return jude_istream_error(stream, "bytes overflow: %s", field->label);

   alloc_size = JUDE_BYTES_ARRAY_T_ALLOCSIZE(size);
   if (size > alloc_size)
      return jude_istream_error(stream, "size too large: %s", field->label);

   if (alloc_size > field->data_size)
      return jude_istream_error(stream, "bytes overflow: %s", field->label);
   bdest = (jude_bytes_array_t*) dest;

   bdest->size = (jude_size_t) size;
   return jude_istream_read(stream, bdest->bytes, size);
}

static bool checkreturn protobuf_dec_string(jude_istream_t *stream, const jude_field_t *field, void *dest)
{
   uint32_t size;
   size_t alloc_size;
   bool status;
   if (!protobuf_dec_varint32(stream, &size))
      return false;

   /* Space for null terminator */
   alloc_size = size + 1;

   if (alloc_size > field->data_size)
      return jude_istream_error(stream, "string overflow: %s", field->label);

   status = jude_istream_read(stream, (uint8_t*) dest, size);
   *((uint8_t*) dest + size) = 0;
   return status;
}

/* Decode string length from stream and return a substream with limited length.
 * Remember to close the substream using jude_close_string_substream().
 */
static bool checkreturn jude_make_string_substream(jude_istream_t *stream, jude_istream_t *substream)
{
   uint32_t size;
   if (!protobuf_dec_varint32(stream, &size))
      return false;

   memcpy(substream, stream, sizeof(jude_istream_t));
   if (substream->bytes_left < size)
      return jude_istream_error(stream, "parent stream too short");

   substream->bytes_left = size;
   stream->bytes_left -= size;
   return true;
}

static void jude_close_string_substream(jude_istream_t *stream, jude_istream_t *substream)
{
   stream->state = substream->state;
   jude_buffer_transfer(&stream->buffer, &substream->buffer);
   stream->last_char = substream->last_char;
   stream->has_error = substream->has_error;
}

/* Decode string length from stream and return a substream with limited length.
 * Remember to close the substream using jude_close_context_substream().
 */
static bool binary_context_substream_open(jude_context_type_t type, jude_istream_t *stream, jude_istream_t *substream)
{
   switch (type)
   {
   case JUDE_CONTEXT_STRING:
   case JUDE_CONTEXT_DELIMITED:
   case JUDE_CONTEXT_REPEATED:
   case JUDE_CONTEXT_SUBMESSAGE:
      return jude_make_string_substream(stream, substream);
   case JUDE_CONTEXT_MESSAGE:
      memcpy(substream, stream, sizeof(jude_istream_t));
      break;
   }
   return true;
}

/*
 * Is the current substream context at an end?
 */
static bool binary_context_substream_is_eof(jude_context_type_t type, jude_istream_t *stream)
{
   return stream->bytes_left == 0;
}

/*
 * Move to next element within a substream context
 * - useful for traversing arrays in wiretypes like JSON
 */
static bool binary_context_substream_next_element(jude_context_type_t type, jude_istream_t *substream)
{
   if (type == JUDE_CONTEXT_DELIMITED)
   {
      // delimited field stream...

   }

   return true;
}

static bool binary_context_substream_close(jude_context_type_t type, jude_istream_t *stream, jude_istream_t *substream)
{
   switch (type)
   {
   case JUDE_CONTEXT_STRING:
   case JUDE_CONTEXT_DELIMITED:
   case JUDE_CONTEXT_REPEATED:
   case JUDE_CONTEXT_SUBMESSAGE:
      jude_close_string_substream(stream, substream);
      break;
   case JUDE_CONTEXT_MESSAGE:
      memcpy(stream, substream, sizeof(jude_istream_t));
      break;
   }

   return true;
}

static bool jude_is_packed(const jude_field_t *field, jude_type_t wire_type)
{
   switch (get_protobuf_wire_type(wire_type))
   {
   case PROTOBUF_WT_STRING:
   case PROTOBUF_WT_VARINT:
      return true;
   default:
      return false;
   }
}

static const jude_decode_transport_t transport =
{
   .dec_bool      = &protobuf_dec_varint,
   .dec_signed    = &protobuf_dec_varint,
   .dec_unsigned  = &protobuf_dec_uvarint,
   .dec_float     = &protobuf_dec_varint,
   .dec_enum      = &protobuf_dec_varint,
   .dec_bitmask   = &protobuf_dec_varint,
   .dec_string    = &protobuf_dec_string,
   .dec_bytes     = &protobuf_dec_bytes,

   .decode_tag = &protobuf_dec_tag,
   .is_packed = &jude_is_packed,
   .skip_field = &protobuf_skip_field,
   .read_raw_value = &read_raw_value,

   .context.open = binary_context_substream_open,
   .context.is_eof = binary_context_substream_is_eof,
   .context.next_element = binary_context_substream_next_element,
   .context.close = binary_context_substream_close
};

const jude_decode_transport_t* jude_decode_transport_protobuf = &transport;
