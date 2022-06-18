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
#include <stdarg.h>
#include <jude/core/cpp/Stream.h>

namespace
{
   size_t ReadCallback(void* user_data, uint8_t* data, size_t length)
   {
      auto is = static_cast<std::istream*>(user_data);
      is->read(reinterpret_cast<char*>(data), length);
      return is->gcount();
   }

   size_t WriteCallback(void* user_data, const uint8_t* data, size_t length)
   {
      auto os = static_cast<std::ostream*>(user_data);
      if (os->write(reinterpret_cast<const char*>(data), length))
      {
         return length;
      }
      return 0;
   }
}

namespace jude 
{
   InputStreamWrapper::InputStreamWrapper(std::istream& input, const jude_decode_transport_t* transport)
      : m_underlyingInput(input)
   {
      jude_istream_create(
         &m_istream,
         transport,
         ReadCallback,
         &m_underlyingInput,
         m_buffer,
         std::size(m_buffer));
   }

   OutputStreamWrapper::OutputStreamWrapper(std::ostream& output, const jude_encode_transport_t* transport)
      : m_underlyingOutput(output)
   {
      jude_ostream_create(
         &m_ostream,
         transport,
         WriteCallback,
         &m_underlyingOutput,
         m_buffer,
         std::size(m_buffer));

      m_ostream.output_recently_cleared_as_null = true;
   }

   OutputStreamWrapper::~OutputStreamWrapper()
   {
      Flush();
   }
   
   void OutputStreamWrapper::Flush()
   {
      jude_ostream_flush(&m_ostream);
      m_underlyingOutput.flush();
   }
}
