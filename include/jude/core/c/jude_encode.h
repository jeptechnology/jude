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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "jude_core.h"

// All wiretype encode functions for any transport type must use functions of this shape
typedef bool (jude_encoder_t)(jude_ostream_t *stream, const jude_field_t *field, const void *src) checkreturn;

// transport layer is either standard GJUDE or JSON
struct jude_encode_transport_t
{
   jude_encoder_t *enc_bool;
   jude_encoder_t *enc_signed;
   jude_encoder_t *enc_unsigned;
   jude_encoder_t *enc_float;
   jude_encoder_t *enc_enum;
   jude_encoder_t* enc_bitmask;
   jude_encoder_t* enc_string;
   jude_encoder_t* enc_bytes;
   jude_encoder_t* enc_object;
   jude_encoder_t* enc_null;

   bool (*encode_tag)(jude_ostream_t *stream, jude_type_t wiretype, const jude_field_t *field);
   bool (*is_packable)(const jude_field_t *field);

   bool (*start_message)(jude_ostream_t *stream);
   bool (*end_message)(jude_ostream_t *stream);
   bool (*array_start)(jude_ostream_t *stream, const jude_field_t *field, const void *pData, size_t count, jude_encoder_t *func);
   bool (*array_end)(jude_ostream_t *stream);

   bool (*next_element)(jude_ostream_t *stream, size_t index);
};

extern const jude_encode_transport_t *jude_encode_transport_protobuf;
extern const jude_encode_transport_t *jude_encode_transport_json;
extern const jude_encode_transport_t *jude_encode_transport_raw;

/***************************
 * Main encoding functions *
 ***************************/

/* Encode a single protocol buffers message from C structure into a stream.
 * Returns true on success, false on any failure.
 * The actual struct pointed to by src_struct must match the description in fields.
 * All required fields in the struct are assumed to have been filled in.
 *
 * Example usage:
 *    MyMessage msg = {};
 *    uint8_t buffer[64];
 *    jude_ostream_t stream;
 *
 *    msg.field1 = 42;
 *    stream = jude_ostream_from_buffer(buffer, sizeof(buffer));
 *    jude_encode(&stream, MyMessage_fields, &msg);
 */
bool jude_encode(jude_ostream_t *stream, const jude_object_t *src_struct);

/* Same as jude_encode, but prepends the length of the message as a varint.
 * Corresponds to writeDelimitedTo() in Google's protobuf API.
 */
bool jude_encode_delimited(jude_ostream_t *stream, const jude_object_t *src_struct);

/* 
 * Encode a single field
 */
bool jude_encode_single_value(jude_ostream_t *stream, const jude_iterator_t *iter);
bool jude_encode_single_element_of_array(jude_ostream_t *stream, const jude_iterator_t *iter, jude_size_t arrayIndex);

bool checkreturn jude_ostream_write_json_tag(jude_ostream_t *stream, const char *tag);
bool checkreturn jude_ostream_write_json_string(jude_ostream_t *stream, const char *buffer, size_t size);

#ifdef __cplusplus
}
#endif
