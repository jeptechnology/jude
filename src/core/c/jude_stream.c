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
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b) (a)<(b)?(a):(b)

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b) (a)>(b)?(a):(b)

/*******************************
 * jude_istream_t implementation *
 *******************************/
static void jude_set_error_in_buffer(jude_buffer_t *buffer, const char *member, const char *format, va_list ap)
{
   if (buffer)
   {
      size_t chars_written = 0;
      size_t max_length = buffer->m_capacity;
      char *write_location = (char *)buffer->m_data;

      if (member)
      {
         chars_written = snprintf(write_location, max_length, "%s: ", member);
      }

      if (chars_written < max_length)
      {
         max_length -= chars_written;
         write_location += chars_written;
         vsnprintf(write_location, max_length, format, ap);
      }
   }
}

unsigned jude_istream_reset_error_to(jude_istream_t* stream, const char* format, ...)
{
   stream->has_error = true;
   va_list ap;
   va_start(ap, format);
   jude_set_error_in_buffer(&stream->error_msg, stream->member, format, ap);
   va_end(ap);

   return 0;
}

unsigned jude_ostream_reset_error_to(jude_ostream_t* stream, const char* format, ...)
{
   stream->has_error = true;

   va_list ap;
   va_start(ap, format);
   jude_set_error_in_buffer(&stream->buffer, stream->member, format, ap);
   va_end(ap);

   return 0;
}

unsigned jude_istream_error(jude_istream_t *stream, const char *format, ...)
{
   if (!stream->has_error)
   {
      stream->has_error = true;

      va_list ap;
      va_start(ap, format);
      jude_set_error_in_buffer(&stream->error_msg, stream->member, format, ap);
      va_end(ap);
   }

   return 0;
}

unsigned jude_ostream_error(jude_ostream_t *stream, const char *format, ...)
{
   if (!stream->has_error)
   {
      stream->has_error = true;

      va_list ap;
      va_start(ap, format);
      jude_set_error_in_buffer(&stream->buffer, stream->member, format, ap);
      va_end(ap);
   }

   return 0;
}

const char* jude_istream_get_error(const jude_istream_t *stream)
{
   if (!stream)
   {
      return "(no stream supplied!)";
   }
   if (!stream->has_error)
   {
      return "(no error)";
   }
   if (stream->error_msg.m_data == 0)
   {
      return "(error)";
   }
   return (const char *)stream->error_msg.m_data;
}

const char* jude_ostream_get_error(const jude_ostream_t *stream)
{
   if (!stream)
   {
      return "(no stream supplied!)";
   }
   if (!stream->has_error)
   {
      return "(no error)";
   }
   if (stream->buffer.m_capacity < 2)
   {
      return "(error)";
   }
   return (const char *)stream->buffer.m_data;
}

bool jude_istream_is_eof(const jude_istream_t *stream)
{
   return stream->has_error || stream->bytes_left == 0;
}

static size_t jude_buffer_bytes_left_to_read(const jude_buffer_t *buffer)
{
   return buffer->m_size - buffer->m_readIndex;
}

static size_t jude_buffer_get_remaining_capacity(const jude_buffer_t *buffer)
{
   return buffer->m_capacity - buffer->m_size;
}

static size_t jude_buffer_read(jude_buffer_t *buffer, uint8_t *output_data, size_t bytes_to_read)
{
   bytes_to_read = MIN(bytes_to_read, jude_buffer_bytes_left_to_read(buffer));
   memcpy(output_data, &buffer->m_data[buffer->m_readIndex], bytes_to_read);
   buffer->m_readIndex += bytes_to_read;
   return bytes_to_read;
}

static size_t jude_buffer_write(jude_buffer_t *buffer, const uint8_t *input_data, size_t bytes_to_write)
{
   bytes_to_write = MIN(bytes_to_write, jude_buffer_get_remaining_capacity(buffer));
   memcpy(&buffer->m_data[buffer->m_size], input_data, bytes_to_write);
   buffer->m_size += bytes_to_write;
   return bytes_to_write;
}

static void set_capacity(jude_buffer_t *buffer, size_t capacity)
{
   buffer->m_capacity = capacity;
}

void jude_buffer_transfer(jude_buffer_t *lhs, const jude_buffer_t *rhs)
{
   memcpy(lhs, rhs, sizeof(jude_buffer_t));
}

static size_t checkreturn buf_read(void *user_data, uint8_t *buf, size_t count)
{
   return 0;
}

static uint32_t jude_discard_input_bytes(jude_istream_t *stream, size_t count)
{
   size_t bytes_yet_to_read = count;

   while (bytes_yet_to_read > 0)
   {
      uint8_t tmp[16];
      size_t byte_count = MIN(sizeof(tmp), bytes_yet_to_read);
      byte_count = jude_istream_read(stream, tmp, byte_count);
      if (byte_count == 0)
      {
         return 0;   // failed to skip
      }
      bytes_yet_to_read -= byte_count;
   }

   return (uint32_t)count;
}

static void replenish_buffer(jude_istream_t *stream)
{
   size_t bytesToRead = MIN(stream->bytes_left, stream->buffer.m_capacity);
   stream->buffer.m_size = (uint32_t)stream->read_callback(stream->state, stream->buffer.m_data, bytesToRead);
   stream->buffer.m_readIndex = 0;
   if (stream->buffer.m_size == 0)
   {
      stream->bytes_left = 0;
   }
}

size_t checkreturn jude_istream_readnext_if_not_eof(jude_istream_t* stream, char* ch)
{
   if (jude_istream_is_eof(stream))
   {
      return false;
   }
   return jude_istream_read(stream, (uint8_t*)ch, 1);
}

size_t checkreturn jude_istream_read(jude_istream_t *stream, uint8_t *buffer, size_t bytesToRead)
{
   if (bytesToRead == 0)
   {
      return 0;
   }

   if (stream->bytes_left == 0)
   {
      return jude_istream_error(stream, "end-of-stream");
   }

   if (buffer == NULL)
   {
      return jude_discard_input_bytes(stream, bytesToRead);
   }

   size_t totalBytesRead = 0;
   while (bytesToRead > 0)
   {
      size_t bytesReadFromBuffer = jude_buffer_read(&stream->buffer, buffer, bytesToRead);
      if (bytesReadFromBuffer > 0)
      {
         bytesToRead -= bytesReadFromBuffer;
         buffer += bytesReadFromBuffer;
         totalBytesRead += bytesReadFromBuffer;
      }

      if (bytesToRead > 0)
      {
         replenish_buffer(stream);
         if (jude_buffer_bytes_left_to_read(&stream->buffer) == 0)
         {
            break; // no more bytes from underlying stream
         }
      }
   }

   if (totalBytesRead > 0)
   {
      stream->last_char = *(buffer - 1);
   }
   stream->bytes_read += totalBytesRead;
   stream->bytes_left -= totalBytesRead;

   return totalBytesRead;
}

/* Read a single byte from input stream. buf may not be NULL.
 * This is an optimization for the varint decoding. */
bool checkreturn jude_istream_readbyte(jude_istream_t *stream, uint8_t *buf)
{
   return jude_istream_read(stream, buf, 1) == 1;
}

void jude_istream_from_readonly(jude_istream_t* stream, const uint8_t* buf, size_t bufsize, char* errbuffer, size_t errorsize)
{
   jude_istream_from_buffer(stream, buf, bufsize);
   if (errbuffer)
   {
      stream->error_msg.m_data = (uint8_t*)errbuffer;
      set_capacity(&stream->error_msg, errorsize);
      stream->error_msg.m_readIndex = 0;
      stream->error_msg.m_size = 0;
   }
}

void jude_istream_from_buffer(jude_istream_t *stream, const uint8_t *buf, size_t bufsize)
{
   memset(stream, 0, sizeof(jude_istream_t));

   stream->read_callback = &buf_read;
   stream->state = NULL;
   stream->bytes_left = bufsize;
   stream->bytes_read = 0;
   stream->always_append_repeated_fields = false;
   stream->transport = jude_decode_transport_json;

   stream->buffer.m_data = buf ? (uint8_t*)buf : (uint8_t*)&stream->last_char;
   set_capacity(&stream->buffer, buf ? bufsize : 1);
   stream->buffer.m_readIndex = 0;
   stream->buffer.m_size = bufsize;
}

/*******************************
 * jude_ostream_t implementation *
 *******************************/

static size_t checkreturn buf_write(void *user_data, const uint8_t *buf, size_t count)
{
   jude_ostream_t *stream = (jude_ostream_t *)user_data;

   // underlying stream will have used the buffer already - and should only call this once
   // buffer is full
   if (stream && stream->buffer.m_capacity > 0)
   {
      set_capacity(&stream->buffer, 0);
      return count;
   }

   return 0;
}

void jude_ostream_for_sizing(jude_ostream_t *stream)
{
   memset(stream, 0, sizeof(jude_ostream_t));
}

void jude_ostream_from_buffer(jude_ostream_t *stream, uint8_t *buf, size_t bufsize)
{
   memset(stream, 0, sizeof(jude_ostream_t));

   stream->write_callback = &buf_write;
   stream->state = stream;
   stream->bytes_written = 0;
   set_capacity(&stream->buffer, buf ? bufsize : 0);
   stream->buffer.m_data = buf;
   stream->buffer.m_readIndex = 0;
   stream->buffer.m_size = 0;
}

void jude_ostream_create(
   jude_ostream_t                *stream,
   const jude_encode_transport_t *transport,
   jude_stream_write_callback_t  *writer,
   void  *user_data,
   char  *buffer,
   size_t buffer_length)
{
   memset(stream, 0, sizeof(jude_ostream_t));

   stream->write_callback = writer;
   stream->transport = transport;
   stream->state = user_data;
   stream->bytes_written = 0;
   set_capacity(&stream->buffer, buffer ? buffer_length : 0);
   stream->buffer.m_data = (uint8_t*)buffer;
   stream->buffer.m_readIndex = 0;
   stream->buffer.m_size = 0;
}

static const uint8_t encoding_table[] =
{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
      'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
      'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
      't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', '+', '/' };

/*
 * Note: This table below is sparsely populated to allow fast base64
 * decoding whereby you pass a character in the range from the above table
 * e.g. 'A', 'Z', '+', etc
 * and it will use an array lookup to get the character.
 *
 * Or to put it mathematically:
 *
 * For all characters C in encoding_table:
 *    C == encoding_table[decoding_table[C]];
 *
 * To understand how the table has been constructed note the following:
 *
 * 1. entries marked '__' are supposed to be blank but are required for the
 * correct position of *real* entries.
 * 2. For each element in the array above, look up the Hex character code and
 * then place the index of that element in the decoding array based on it's hex
 * code.
 * e.g. For 'J', it is the 10th element of the encoding table so that's an index of 9 (zero based).
 * ASCII code for 'J' is 0x4A - hence we populate the decoding_table[0x4A] with value of 9.
 *
 */
#define __ 0

static const uint8_t decoding_table[] =
{
   /*       0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  */
   /* 0 */__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
   /* 1 */__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
   /* 2 */__, __, __, __, __, __, __, __, __, __, __, 62, __, __, __, 63,
   /* 3 */52, 53, 54, 55, 56, 57, 58, 59, 60, 61, __, __, __,  0, __, __, // Note: Decode of '=' should be '\0' so we add entry at 0x3D
   /* 4 */__,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   /* 5 */15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, __, __, __, __, __,
   /* 6 */__, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   /* 7 */41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, __, __, __, __, __,

// NOTE: We only need the first 0x7F entries as we don't expect base64
// encoded chars to be any higher. If they are the behaviour is already undefined!
   /* 8 / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / 9 / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / A / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / B / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / C / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / D / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / E / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
    / F / __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, */
};

bool checkreturn jude_ostream_flush(jude_ostream_t *stream)
{
   if (!stream || stream->has_error)
   {
      return false;
   }

   size_t total_bytes_written = 0;

   while (total_bytes_written < stream->buffer.m_size)
   {
      size_t bytes_written = stream->write_callback(stream->state,
                                                    &stream->buffer.m_data[total_bytes_written],
                                                    stream->buffer.m_size - total_bytes_written);
      if (bytes_written == 0)
      {
         break; // not able to write any bytes => maybe failure
      }

      total_bytes_written += bytes_written;
   }

   stream->buffer.m_size = 0;

   return true;
}

static size_t buffered_write(jude_ostream_t *stream, const uint8_t *buf, size_t count)
{
   size_t total_bytes_written = 0;
   while (count > 0)
   {
      if (jude_buffer_get_remaining_capacity(&stream->buffer) == 0)
      {
         if (!jude_ostream_flush(stream))
         {
            return jude_ostream_error(stream, "io error");
         }
      }

      size_t bytes_written = jude_buffer_write(&stream->buffer, buf, count);
      count -= bytes_written;
      buf += bytes_written;
      total_bytes_written += bytes_written;
   }
   return total_bytes_written;
}

size_t checkreturn jude_ostream_write(jude_ostream_t *stream, const uint8_t *buf, size_t count)
{
   if (stream->write_callback == NULL)
   {
      // just a counting output stream
      stream->bytes_written += count;
      return count;
   }

   size_t total_bytes_written = 0;

   if (stream->buffer.m_capacity == 0)
   {
      // unbuffered
      total_bytes_written = stream->write_callback(stream->state, buf, count);
   }
   else
   {
      total_bytes_written = buffered_write(stream, buf, count);
   }

   stream->bytes_written += total_bytes_written;
   return total_bytes_written;
}

size_t jude_ostream_printf(jude_ostream_t* stream, size_t count, const char* format, ...)
{
   if (format == NULL)
   {
      return jude_ostream_error(stream, "Null format specified");
   }

   va_list ap;
   va_start(ap, format);
   size_t length = vsnprintf(NULL, 0, format, ap);
   va_end(ap);

   if (length > 0)
   {
      char stackBuffer[64]; // for speed on small transactions
      char* buffer = (length < sizeof(stackBuffer) ? stackBuffer : (char *)malloc((size_t)length + 1));

      va_start(ap, format);
      vsnprintf(buffer, (size_t)length + 1, format, ap);
      va_end(ap);

      length = (int)jude_ostream_write(stream, (const uint8_t*)buffer, length);
      
      if (buffer != stackBuffer)
      {
         free(buffer);
      }
   }

   return length > 0 ? length : 0;
}

bool json_base64_write(jude_ostream_t *stream, const uint8_t *data, size_t count)
{
   static size_t padding_table[] = { 0, 2, 1 };

   const size_t padding = padding_table[count % 3];
   uint32_t i = 0;

   while (i < count)
   {
      uint32_t octet_a = i < count ? data[i++] : 0;
      uint32_t octet_b = i < count ? data[i++] : 0;
      uint32_t octet_c = i < count ? data[i++] : 0;

      uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

      uint8_t encoded_data[4];

      encoded_data[0] = encoding_table[(triple >> (3 * 6)) & 0x3F];
      encoded_data[1] = encoding_table[(triple >> (2 * 6)) & 0x3F];
      encoded_data[2] = encoding_table[(triple >> (1 * 6)) & 0x3F];
      encoded_data[3] = encoding_table[(triple >> (0 * 6)) & 0x3F];

      if (i >= count)
      {
         if (!jude_ostream_write(stream, encoded_data, 4 - padding))
         {
            return jude_ostream_error(stream, "base64 encode error");
         }

         if (padding && !jude_ostream_write(stream, (uint8_t*)"==", padding))
         {
            return jude_ostream_error(stream, "base64 encode error");
         }
      }
      else
      {
         if (!jude_ostream_write(stream, encoded_data, 4))
         {
            return jude_ostream_error(stream, "stream full");
         }
      }
   }

   return true;
}

jude_size_t json_base64_read(jude_istream_t *stream, uint8_t *decoded_data, jude_size_t max)
{
   jude_size_t decode_idx = 0;
   bool end_of_bytes = false;
   uint8_t tmp_decode_buffer[] = "===";

   while (decode_idx < max && !end_of_bytes)
   {
      uint8_t data[] = "====";
      int data_idx = 0;
      while (data_idx < 4)
      {
         if (!jude_istream_read(stream, &data[data_idx], 1))
         {
            return jude_istream_error(stream, "base64 encode error");
         }

         if (data[data_idx] == '"')
         {
            data[data_idx] = '=';
            end_of_bytes = true;
            break;
         }
         data_idx++;
      }

      {
         uint32_t sextet_a = data[0] == '=' ? 0 : decoding_table[data[0]];
         uint32_t sextet_b = data[1] == '=' ? 0 : decoding_table[data[1]];
         uint32_t sextet_c = data[2] == '=' ? 0 : decoding_table[data[2]];
         uint32_t sextet_d = data[3] == '=' ? 0 : decoding_table[data[3]];

         uint32_t triple = (sextet_a << (3 * 6))
                         | (sextet_b << (2 * 6))
                         | (sextet_c << (1 * 6))
                         | (sextet_d << (0 * 6));

         uint8_t tmp_idx = 0;

         if (decode_idx + 0 < max && data[1] != '=') tmp_decode_buffer[tmp_idx++] = (triple >> 2 * 8) & 0xFF;
         if (decode_idx + 1 < max && data[2] != '=') tmp_decode_buffer[tmp_idx++] = (triple >> 1 * 8) & 0xFF;
         if (decode_idx + 2 < max && data[3] != '=') tmp_decode_buffer[tmp_idx++] = (triple >> 0 * 8) & 0xFF;

         if (!stream->field_got_changed && memcmp(&decoded_data[decode_idx], tmp_decode_buffer, tmp_idx) != 0)
         {
            stream->field_got_changed = true;
         }
         memcpy(&decoded_data[decode_idx], tmp_decode_buffer, tmp_idx);
         decode_idx += tmp_idx;
      }
   }

   if (!end_of_bytes)
   {
      // force error in caller
      decode_idx = max + 1;
   }
   return decode_idx;
}

jude_istream_t jude_istream_substream(jude_istream_t *stream, size_t count)
{
   jude_istream_t substream;

   memcpy(&substream, stream, sizeof(jude_istream_t));

   substream.read_callback = stream->read_callback;
   substream.state = stream;
   substream.bytes_left = count;
   substream.bytes_read = 0;

   return substream;
}

void jude_istream_create(
      jude_istream_t *istream,
      const jude_decode_transport_t *transport,
      jude_stream_read_callback_t   *reader,
      void  *user_data,
      char  *buffer,
      size_t buffer_length)
{
   memset(istream, 0, sizeof(jude_istream_t));

   istream->transport = transport;
   istream->read_callback = reader;
   istream->state = user_data;
   istream->bytes_left = UINT32_MAX; // let the underlying callback decide when we are EOF
   istream->bytes_read = 0;
   istream->buffer.m_data = (uint8_t*) (buffer ? buffer : &istream->last_char);
   set_capacity(&istream->buffer, buffer ? buffer_length : 1);
   istream->buffer.m_readIndex = 0;
   istream->buffer.m_size = 0;

   if (buffer)
   {
      istream->error_msg.m_data = (uint8_t*)buffer;
      set_capacity(&istream->error_msg, buffer_length );
   }
}
