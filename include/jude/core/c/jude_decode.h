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

#include "jude_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
   JUDE_CONTEXT_REPEATED,
   JUDE_CONTEXT_STRING,
   JUDE_CONTEXT_MESSAGE,
   JUDE_CONTEXT_SUBMESSAGE,
   JUDE_CONTEXT_DELIMITED
} jude_context_type_t;

// All wiretype decode functions for any transport type must use functions of this shape
typedef bool (jude_decoder_t)(jude_istream_t *stream, const jude_field_t *field, void *dest) checkreturn;

// transport layer can be JSON, Protobuf (binary) or RAW (memory)
struct jude_decode_transport_t
{
   jude_decoder_t* dec_bool;
   jude_decoder_t* dec_signed;
   jude_decoder_t* dec_unsigned;
   jude_decoder_t* dec_float;
   jude_decoder_t* dec_enum;
   jude_decoder_t* dec_bitmask;
   jude_decoder_t* dec_string;
   jude_decoder_t* dec_bytes;

   bool (*decode_tag)(jude_istream_t *stream, jude_object_t *object, jude_type_t *wire_type, uint32_t *tag, bool *eof);
   bool (*is_packed)(const jude_field_t *field, jude_type_t wire_type);
   bool (*skip_field)(jude_istream_t *stream, jude_type_t wire_type);
   bool (*read_raw_value)(jude_istream_t *stream, jude_type_t wire_type, uint8_t *buf, size_t *size);

   struct
   {
      bool (*open)(jude_context_type_t type, jude_istream_t *stream, jude_istream_t *substream);
      bool (*is_eof)(jude_context_type_t type, jude_istream_t *substream);
      bool (*next_element)(jude_context_type_t type, jude_istream_t *substream);
      bool (*close)(jude_context_type_t type, jude_istream_t *stream, jude_istream_t *substream);
   } context;
};

extern const jude_decode_transport_t *jude_decode_transport_protobuf;
extern const jude_decode_transport_t *jude_decode_transport_json;
extern const jude_decode_transport_t *jude_decode_transport_raw;

/***************************
 * Main decoding functions *
 ***************************/

/* Decode a single protocol buffers message from input stream into a C structure.
 * Returns true on success, false on any failure.
 * The actual struct pointed to by dest must match the description in fields.
 * Callback fields of the destination structure must be initialized by caller.
 * All other fields will be initialized by this function.
 *
 * Example usage:
 *    MyMessage msg = {};
 *    uint8_t buffer[64];
 *    jude_istream_t stream;
 *    
 *    // ... read some data into buffer ...
 *
 *    stream = jude_istream_from_buffer(buffer, count);
 *    jude_decode(&stream, MyMessage_fields, &msg);
 */
bool jude_decode(jude_istream_t *stream, jude_object_t *dest_struct);

/* Same as jude_decode, except does not initialize the destination structure
 * to default values. This is slightly faster if you need no default values
 * and just do memset(struct, 0, sizeof(struct)) yourself.
 *
 * This can also be used for 'merging' two messages, i.e. update only the
 * fields that exist in the new message.
 *
 * Note: If this function returns with an error, it will not release any
 * dynamically allocated fields. You will need to call jude_release() yourself.
 */
bool jude_decode_noinit(jude_istream_t *stream, jude_object_t *dest_struct);

/* Same as jude_decode, except expects the stream to start with the message size
 * encoded as varint. Corresponds to parseDelimitedFrom() in Google's
 * protobuf API.
 */
bool jude_decode_delimited(jude_istream_t *stream, jude_object_t *dest_struct);

/*
 * Decode a single field
 */
bool jude_decode_single_field(jude_istream_t *stream, jude_iterator_t *iter);

/*
 * Decode a targeted element
 * Can be used on single fields if index == 0
 */
bool decode_field_element(jude_istream_t* stream, jude_iterator_t* iter, jude_size_t index);

#ifdef __cplusplus
}
#endif
