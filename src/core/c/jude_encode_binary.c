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
static bool checkreturn protobuf_enc_varint(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_enc_uvarint(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_enc_svarint(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_enc_fixed32(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_enc_fixed64(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_enc_bytes(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_enc_string(jude_ostream_t *stream, const jude_field_t *field, const void *src);
static bool checkreturn protobuf_encode_varint(jude_ostream_t *stream, uint64_t value);

/***************************
 * Binary Helper functions *
 ***************************/

static bool checkreturn protobuf_encode_object(jude_ostream_t *stream, const jude_object_t *src_struct)
{
   /* First calculate the message size using a non-writing substream. */
   jude_ostream_t sizing_stream;
   jude_ostream_for_sizing(&sizing_stream);

   sizing_stream.transport = stream->transport;
   sizing_stream.read_access_control = stream->read_access_control;
   sizing_stream.read_access_control_ctx = stream->read_access_control_ctx;
   sizing_stream.state = stream->state;

   if (!jude_encode(&sizing_stream, src_struct))
   {
      return jude_ostream_error(stream, stream->member, "Could not size substream");
   }

   size_t expected_size = sizing_stream.bytes_written;

   if (!protobuf_encode_varint(stream, (uint64_t) expected_size))
      return false;

   if (stream->write_callback == NULL)
      return jude_ostream_write(stream, NULL, expected_size); /* Just sizing */

   size_t bytes_written_origin = stream->bytes_written;

   /* Verify that a callback doesn't write more than what it did the first time. */
   if (!jude_encode(stream, src_struct))
   {
      return false;
   }

   size_t encoded_submessage_bytes_written = (stream->bytes_written - bytes_written_origin);
   if (encoded_submessage_bytes_written != expected_size)
   {
      return jude_ostream_error(stream, stream->member, "Submessage expected size %d but %d bytes written", expected_size, encoded_submessage_bytes_written);
   }

   return true;
}

static bool checkreturn protobuf_encode_varint(jude_ostream_t *stream, uint64_t value)
{
   uint8_t buffer[10];
   size_t i = 0;

   if (value == 0)
      return jude_ostream_write(stream, (uint8_t*) &value, 1);

   while (value)
   {
      buffer[i] = (uint8_t) ((value & 0x7F) | 0x80);
      value >>= 7;
      i++;
   }
   buffer[i - 1] &= 0x7F; /* Unset top bit on last byte */

   return jude_ostream_write(stream, buffer, i);
}

static bool checkreturn protobuf_encode_svarint(jude_ostream_t *stream, int64_t value)
{
   uint64_t zigzagged;
   if (value < 0)
      zigzagged = ~((uint64_t) value << 1);
   else
      zigzagged = (uint64_t) value << 1;

   return protobuf_encode_varint(stream, zigzagged);
}

static bool checkreturn protobuf_encode_fixed32(jude_ostream_t *stream, const void *value)
{
#ifdef __BIG_ENDIAN__
   const uint8_t *bytes = value;
   uint8_t lebytes[4];
   lebytes[0] = bytes[3];
   lebytes[1] = bytes[2];
   lebytes[2] = bytes[1];
   lebytes[3] = bytes[0];
   return jude_ostream_write(stream, lebytes, 4);
#else
   return jude_ostream_write(stream, (const uint8_t*) value, 4);
#endif
}

static bool checkreturn protobuf_encode_fixed64(jude_ostream_t *stream, const void *value)
{
#ifdef __BIG_ENDIAN__
   const uint8_t *bytes = value;
   uint8_t lebytes[8];
   lebytes[0] = bytes[7];
   lebytes[1] = bytes[6];
   lebytes[2] = bytes[5];
   lebytes[3] = bytes[4];
   lebytes[4] = bytes[3];
   lebytes[5] = bytes[2];
   lebytes[6] = bytes[1];
   lebytes[7] = bytes[0];
   return jude_ostream_write(stream, lebytes, 8);
#else
   return jude_ostream_write(stream, (const uint8_t*) value, 8);
#endif
}

static bool protobuf_is_packable_field(const jude_field_t *field)
{
   // We are not going to pack any fields - TODO in future implementation
   return false;
}

static bool checkreturn protobuf_encode_tag(jude_ostream_t *stream, jude_type_t wiretype, const jude_field_t *field)
{
   uint32_t field_number = field->tag;
   uint64_t tag = ((uint64_t) field_number << 3) | wiretype;
   return protobuf_encode_varint(stream, tag);
}

static bool checkreturn protobuf_encode_string(jude_ostream_t *stream, const uint8_t *buffer, size_t size)
{
   if (!protobuf_encode_varint(stream, (uint64_t) size))
      return false;

   return jude_ostream_write(stream, buffer, size);
}

/* Field encoders */

static bool checkreturn protobuf_enc_varint(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   int64_t value = 0;

   /* Cases 1 and 2 are for compilers that have smaller types for bool
    * or enums, and for int_size option. */
   switch (field->data_size)
   {
   case 1:
      value = *(const int8_t*) src;
      break;
   case 2:
      value = *(const int16_t*) src;
      break;
   case 4:
      value = *(const int32_t*) src;
      break;
   case 8:
      value = *(const int64_t*) src;
      break;
   default:
      return jude_ostream_error(stream, "invalid data_size");
   }

   return protobuf_encode_varint(stream, (uint64_t) value);
}

static bool checkreturn protobuf_enc_uvarint(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   uint64_t value = 0;

   switch (field->data_size)
   {
   case 1:
      value = *(const uint8_t*) src;
      break;
   case 2:
      value = *(const uint16_t*) src;
      break;
   case 4:
      value = *(const uint32_t*) src;
      break;
   case 8:
      value = *(const uint64_t*) src;
      break;
   default:
      return jude_ostream_error(stream, "invalid data_size");
   }

   return protobuf_encode_varint(stream, value);
}

static bool checkreturn protobuf_enc_svarint(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   int64_t value = 0;

   switch (field->data_size)
   {
   case 1:
      value = *(const int8_t*) src;
      break;
   case 2:
      value = *(const int16_t*) src;
      break;
   case 4:
      value = *(const int32_t*) src;
      break;
   case 8:
      value = *(const int64_t*) src;
      break;
   default:
      return jude_ostream_error(stream, "invalid data_size");
   }

   return protobuf_encode_svarint(stream, value);
}

static bool checkreturn protobuf_enc_fixed64(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   JUDE_UNUSED(field);
   return protobuf_encode_fixed64(stream, src);
}

static bool checkreturn protobuf_enc_fixed32(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   JUDE_UNUSED(field);
   return protobuf_encode_fixed32(stream, src);
}

static bool checkreturn protobuf_enc_bytes(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   const jude_bytes_array_t *bytes = (const jude_bytes_array_t*) src;

   if (src == NULL)
   {
      /* Threat null pointer as an empty bytes field */
      return protobuf_encode_string(stream, NULL, 0);
   }

   if (JUDE_BYTES_ARRAY_T_ALLOCSIZE(bytes->size) > field->data_size)
   {
      return jude_ostream_error(stream, "bytes size exceeded");
   }

   return protobuf_encode_string(stream, bytes->bytes, bytes->size);
}

static bool checkreturn protobuf_enc_string(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   size_t size = 0;
   size_t max_size = field->data_size;
   const char *p = (const char*) src;

   if (src == NULL)
   {
      size = 0; /* Threat null pointer as an empty string */
   }
   else
   {
      /* strnlen() is not always available, so just use a loop */
      while (size < max_size && *p != '\0')
      {
         size++;
         p++;
      }
   }

   return protobuf_encode_string(stream, (const uint8_t*) src, size);
}

static bool checkreturn protobuf_enc_submessage(jude_ostream_t *stream, const jude_field_t *field, const void *src)
{
   if (field->details.sub_rtti == NULL)
   {
      return jude_ostream_error(stream, "invalid field descriptor");
   }

   if (field->details.sub_rtti != ((jude_object_t *) src)->__rtti)
   {
      return jude_ostream_error(stream, "Sub message type info not initialised");
   }

   return protobuf_encode_object(stream, (jude_object_t *) src);
}

static bool protobuf_array_start(jude_ostream_t *stream, const jude_field_t *field, const void *pData, size_t count, jude_encoder_t *func)
{
   size_t size;
   const void *p;

   /* Determine the total size of packed array. */
   size_t i;
   jude_ostream_t sizestream;
   jude_ostream_for_sizing(&sizestream);

   p = pData;
   for (i = 0; i < count; i++)
   {
      if (!func(&sizestream, field, p))
         return false;
      p = (const char*) p + field->data_size;
   }

   size = sizestream.bytes_written;

   if (!protobuf_encode_varint(stream, (uint64_t) size))
      return false;

   return true;
}

static bool protobuf_array_end(jude_ostream_t *stream)
{
   return true;
}

static bool protobuf_start_message(jude_ostream_t *stream)
{
   return true;
}

static bool protobuf_end_message(jude_ostream_t *stream)
{
   return true;
}

static bool protobuf_next_element(jude_ostream_t *stream, size_t index)
{
   return true;
}

const jude_encode_transport_t transport =
{
   .enc_bool     = protobuf_enc_uvarint,
   .enc_signed   = protobuf_enc_varint,
   .enc_unsigned = protobuf_enc_uvarint,
   .enc_float    = protobuf_enc_varint,
   .enc_enum     = protobuf_enc_uvarint,
   .enc_bitmask  = protobuf_enc_uvarint,
   .enc_string   = protobuf_enc_string,
   .enc_bytes    = protobuf_enc_bytes,
   .enc_object   = protobuf_enc_submessage,

   .encode_tag = protobuf_encode_tag,
   .is_packable = protobuf_is_packable_field,

   .start_message = protobuf_start_message,
   .end_message = protobuf_end_message,

   .array_start = protobuf_array_start,
   .array_end = protobuf_array_end,

   .next_element = protobuf_next_element
};

const jude_encode_transport_t * jude_encode_transport_protobuf = &transport;
