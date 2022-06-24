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

#include <jude/jude_core.h>
#include <string>
#include <functional>
#include <optional>
#include <iostream>

#ifdef __GNUC__
#define CHECK_FORMAT(formatParamIndex,argsBeginParamIndex) __attribute__ ((format (printf, formatParamIndex,argsBeginParamIndex)))
#else
#define CHECK_FORMAT(formatParamIndex,argsBeginParamIndex)
#endif

namespace jude 
{
   static constexpr size_t DefaultBufferSize = 4096;

   struct InputStreamWrapper
   {
      std::vector<char> m_buffer;
      std::istream&  m_underlyingInput; // where we really get our data from
      jude_istream_t m_istream; // the structure for the low level C code to use

      InputStreamWrapper(std::istream& input, size_t bufferSize = DefaultBufferSize, const jude_decode_transport_t* transport = jude_decode_transport_json);
   };

   struct OutputStreamWrapper
   {
      std::vector<char> m_buffer;
      std::ostream&  m_underlyingOutput; // where we want our data to eventually go
      jude_ostream_t m_ostream;          // wrapper used by lowl level C code
      
      OutputStreamWrapper(std::ostream& output, size_t bufferSize = DefaultBufferSize, const jude_encode_transport_t* transport = jude_encode_transport_json);
      ~OutputStreamWrapper();

      void Flush();

      // Use this to set the type of encoding that you want - e.g. JSON, protobuf, etc
      void SetOutputEncoding(const jude_encode_transport_t* transport);
   };

   using EmbeddedJSONWriter = std::function< void(std::ostream&) >;
}
