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

#include "jude_common.h"
#include "jude_filter.h"

typedef void (access_control_callback_t)(void *user_data, const jude_object_t*, jude_filter_t*);
typedef bool (unknown_field_callback_t)(void *user_data, const char *unknownFieldName, const char *fieldData);
typedef bool (extra_output_callback_t)(void *user_data, const char **extraFieldName, const char **fieldData);

typedef size_t (jude_stream_read_callback_t)(void *user_data, uint8_t* buffer, size_t max_length);
typedef size_t (jude_stream_write_callback_t)(void *user_data, const uint8_t* data, size_t length);

typedef struct jude_buffer_t
{
   uint8_t *m_data;
   size_t   m_capacity;
   size_t   m_size;
   size_t   m_readIndex;
} jude_buffer_t;

void jude_buffer_transfer(jude_buffer_t *lhs, const jude_buffer_t *rhs);

struct jude_ostream_t
{
   const jude_encode_transport_t *transport;
   jude_stream_write_callback_t  *write_callback;

   void  *state; /* Free field for use by callback implementation. */
   size_t bytes_written; /* Number of bytes written so far. */
   jude_buffer_t buffer;
   const char *member;      // current field being encoded
   bool  has_error;
   bool  suppress_first_tag;      // don't output first tag
   /*
    * Read access control callback (optionally NULL) will be called whenever fields are accessed during encoding
    * A null filter allows read access to all fields.
    * Fields filtered out are not read.
    */
   access_control_callback_t *read_access_control;
   void* read_access_control_ctx; // context pointer

   // The following field can be used by the caller to output extra fields if they so wish
   extra_output_callback_t *extra_output_callback;
   void* extra_output_callback_ctx; // context pointer
};

struct jude_istream_t
{
   const jude_decode_transport_t *transport;
   jude_stream_read_callback_t   *read_callback;
   void *        state;      /* Free field for use by callback implementation */
   size_t        bytes_read;
   jude_buffer_t buffer;
   const char *  member;     /* name of current field being decoded */
   size_t        bytes_left; /* used for limiting number of bytes read from a stream */
   char          last_char;  /* last char read - required for some transports */
   bool          has_error; /* set whenever a field is changed by a function */
   jude_buffer_t error_msg; /* some input streams have ROM data in the buffer so we need another buffer for error messages */
   bool          field_got_changed; /* set whenever a field is changed by a function */
   bool          field_got_nulled; /* set whenever a field is patch to "null" by a function */
   bool          always_append_repeated_fields;

   /*
    * Write access_filter callback (optionally NULL) will be called whenever fields are accessed during decoding
    * A null filter allows access to all fields.
    * Fields filtered out are not written to.
    */
   access_control_callback_t *write_access_control;
   void* write_access_control_ctx; // context pointer

   /*
    * This callback is useful if you want to interpret fields that are unknown to the schema 
    */
   unknown_field_callback_t *unknown_field_callback;
};

unsigned jude_istream_error(jude_istream_t *stream, const char *format, ...);
unsigned jude_ostream_error(jude_ostream_t *stream, const char *format, ...);
unsigned jude_istream_reset_error_to(jude_istream_t* stream, const char* format, ...);
unsigned jude_ostream_reset_error_to(jude_ostream_t* stream, const char* format, ...);
const char* jude_istream_get_error(const jude_istream_t *stream);
const char* jude_ostream_get_error(const jude_ostream_t *stream);

void jude_ostream_from_buffer(jude_ostream_t *stream, uint8_t *buf, size_t bufsize);
void jude_ostream_for_sizing(jude_ostream_t *stream);
void jude_ostream_create(
   jude_ostream_t *stream,
   const jude_encode_transport_t *transport,
   jude_stream_write_callback_t   *reader,
   void  *user_data,
   char  *buffer,
   size_t buffer_length);

/* Function to write into a jude_ostream_t stream. You can use this if you need
 * to append or prepend some custom objects to the message.
 */
size_t jude_ostream_write(jude_ostream_t *stream, const uint8_t *buf, size_t count);
size_t jude_ostream_printf(jude_ostream_t* stream, size_t count, const char* format, ...);
bool   jude_ostream_flush(jude_ostream_t *stream);

/*
 * Utility Base64 codec functions for JSON wire type
 */
bool json_base64_write(jude_ostream_t *stream, const uint8_t *data, size_t count);
jude_size_t json_base64_read(jude_istream_t *stream, uint8_t *decoded_data, jude_size_t max);

/**************************************
 * Functions for manipulating streams *
 **************************************/

/* Create an input stream for reading from a memory buffer.
 *
 * Alternatively, you can use a custom stream that reads directly from e.g.
 * a file or a network socket.
 */
void jude_istream_from_buffer(jude_istream_t *stream, const uint8_t *buf, size_t bufsize);
void jude_istream_from_readonly(jude_istream_t* stream, const uint8_t* buf, size_t bufsize, char* errbuffer, size_t errorsize);
void jude_istream_create(
      jude_istream_t *stream,
      const jude_decode_transport_t *transport,
      jude_stream_read_callback_t   *reader,
      void  *user_data,
      char  *buffer,
      size_t buffer_length);

/* Function to read from a jude_istream_t. You can use this if you need to
 * read some custom object data, or to read data in field callbacks.
 */
size_t jude_istream_read(jude_istream_t *stream, uint8_t *buf, size_t count);
bool checkreturn jude_istream_readbyte(jude_istream_t *stream, uint8_t *buf);
size_t checkreturn jude_istream_readnext_if_not_eof(jude_istream_t* stream, char* ch);
bool jude_istream_is_eof(const jude_istream_t *stream);

#ifdef __cplusplus
}
#endif
