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

#ifdef __GNUC__
#define CHECK_FORMAT(formatParamIndex,argsBeginParamIndex) __attribute__ ((format (printf, formatParamIndex,argsBeginParamIndex)))
#else
#define CHECK_FORMAT(formatParamIndex,argsBeginParamIndex)
#endif

namespace jude 
{
   class InputStreamInterface
   {
      size_t m_totalBytesRead{ 0 }; // total bytes read
      size_t m_lastBytesRead{ (size_t)-1 };  // bytes read in last call to Read()
      jude_istream_t m_istream;

      static size_t ReadCallback(void* user_data, uint8_t* data, size_t length);

   public:
      InputStreamInterface(const jude_decode_transport_t* transport = jude_decode_transport_json);

      // Use this to allow input stream to be buffered using the given buffer
      void SetInputBuffer(char* buffer, size_t bufferLen);
      void SetInputBuffer(std::vector<char>& buffer) { SetInputBuffer(buffer.data(), buffer.capacity()); }
      void ClearBuffer();
      
      // Use this to set the type of decoding transport that you want - e.g. JSON, protobuf, etc
      void SetInputDecoding(const jude_decode_transport_t* transport);

      // The low level stream is used by the C level api
      jude_istream_t* GetLowLevelInputStream() { return &m_istream; }

      size_t GetTotalBytesRead() const { return m_totalBytesRead; }
      size_t GetLastBytesRead() const { return m_lastBytesRead; }
      virtual size_t Read(uint8_t* data, size_t length);

      bool HasInputError() const { return m_istream.has_error; }
      void SetInputErrorMsg(const char* error) { jude_istream_error(&m_istream, error); }
      const char * GetInputErrorMsg() const { return jude_istream_get_error(&m_istream); }

      bool IsEof() const
      {
         return HasInputError() || GetLastBytesRead() == 0;
      }

      // Convenience functions
      size_t Read(char *buffer, size_t maxBytesToRead) { return Read(reinterpret_cast<uint8_t*>(buffer), maxBytesToRead); }
      size_t Read(char& datum) { return Read(&datum, 1); }
      size_t Read(uint8_t &datum) { return Read(reinterpret_cast<char&>(datum)); }
      size_t Read(int8_t &datum) { return Read(reinterpret_cast<char&>(datum)); }
      size_t Read(std::vector<uint8_t>& destination); // destination must have some capacity
      size_t ReadLineN(size_t maxBytesToRead, std::string& line, char delimiter = 0);
      size_t ReadLine(std::string& line, char delimiter = 0); // up to line ending
      size_t ReadWord(std::string& line, char delimiter)
      {
         return ReadLine(line, delimiter);
      }

      InputStreamInterface& operator >> (std::string& data)
      {
         // NOTE: Reading a string will read until the next space or newline
         // If you want to read an entire line use the ReadLine function
         ReadWord(data, ' ');
         return *this;
      }

      template<typename T>
      InputStreamInterface& operator >> (T& data)
      { 
         Read(data);
         return *this;
      }

      operator bool() const
      {
         return !HasInputError();
      }

   protected:
      // To implement:
      // Read up to 'length' bytes from the stream and store them in the memory pointed to by data
      // Return the number of bytes successfully read, 0 on EOF / error
      virtual size_t ReadImpl(uint8_t* data, size_t length) = 0;
   };

   class RomInputStream : public InputStreamInterface
   {
      const uint8_t* m_data;
      size_t m_bytesLeft;
      char m_buffer[128]; // without a buffer we can't log descriptive errors so put a small one here always

   protected:
      virtual size_t ReadImpl(uint8_t* data, size_t length) override;

   public:
      explicit RomInputStream(const char* data); // uses strlen
      RomInputStream(const char* data, size_t length); // truncate at 'length' bytes
      RomInputStream(const uint8_t* data, size_t length); // truncate at 'length' bytes
      explicit RomInputStream(const std::string& data) : RomInputStream(data.c_str(), data.length()) {}

      void SetConstData(const uint8_t *data, size_t length);
      void SetConstData(const char *data) { SetConstData(reinterpret_cast<const uint8_t*>(data), strlen(data)); }
      void SetConstData(const char *data, size_t length) { SetConstData((uint8_t*)data, length); }
      void SetConstData(const std::string& s) { SetConstData(s.c_str(), s.length()); }
      size_t GetBytesLeft() const { return m_bytesLeft; }


   };

   class OutputStreamInterface
   {
      static constexpr size_t DefaultWriteChunkSize = 1024;
      jude_ostream_t m_ostream;

      static size_t WriteCallback(void* user_data, const uint8_t* data, size_t length);

   public:
      OutputStreamInterface(const jude_encode_transport_t* transport = jude_encode_transport_json);

      // Use this to allow output stream to be buffered using the given buffer
      void SetOutputBuffer(char* buffer, size_t bufferLen);
      void SetOutputBuffer(std::vector<char>& buffer) { SetOutputBuffer(buffer.data(), buffer.capacity()); }

      // Use this to set the type of encoding that you want - e.g. JSON, protobuf, etc
      void SetOutputEncoding(const jude_encode_transport_t* transport);

      // The low level stream is used by the C level api
      jude_ostream_t* GetLowLevelOutputStream() { return &m_ostream; }

      size_t GetTotalBytesWritten() const { return m_ostream.bytes_written; }
      
      size_t Write(const uint8_t * data, size_t length);
      size_t Print(const char* text);
      size_t Printf(size_t max, const char* format, ...) CHECK_FORMAT(3,4);

      virtual bool Flush(); // flush any buffered output to underlying stream - you can override this if you need to perform special operations

      bool HasOutputError() const { return m_ostream.has_error; }
      void SetOutputErrorMsg(const char* error) { jude_ostream_error(&m_ostream, error); }
      const char* GetOutputErrorMsg() const { return jude_ostream_get_error(&m_ostream); }

      // Convenience functions
      size_t Write(uint8_t datum) { return Write(&datum, 1); }
      size_t Write(char datum) { return Write(static_cast<uint8_t>(datum)); }
      size_t Write(const char *data) { return Write(reinterpret_cast<const uint8_t*>(data), strlen(data)); }
      size_t Write(const char *data, size_t length) { return Write(reinterpret_cast<const uint8_t*>(data), length); }
      size_t Write(const std::string &data) { return Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.length()); }
      size_t Write(const std::vector<uint8_t> &data) { return Write(data.data(), data.size()); }
      
      template <class ... Args>
      size_t WriteFormat(size_t maxBytesToWrite, const char *format, Args ... args)
      {
         return Printf(maxBytesToWrite, format, args...);
      }

      size_t Pipe(InputStreamInterface &inputStream,
                         size_t chunkSizeBytes = DefaultWriteChunkSize, 
                         std::optional<size_t> expectedInputSize = std::nullopt,
                         std::function<void(size_t)> progressCallback = nullptr);
      
      template<typename T> 
      OutputStreamInterface& operator << (T data)
      { 
         Write(data);
         return *this;
      }

      operator bool() const
      {
         return !HasOutputError();
      }

   protected:
      // To implement:
      // Write 'length' bytes from the memory pointed to by data
      // Return the number of bytes successfully written, -1 on error
      virtual size_t WriteImpl(const uint8_t * data, size_t length) = 0;

   public:
      ///////////////////////////////////////////////////////////////////////////////
      // Protobuf backwards compatibility
      struct OutputStreamResult
      {
         OutputStreamInterface& stream;
         operator bool() const { return !stream.HasOutputError(); }
         auto GetError() { return stream.GetOutputErrorMsg(); }
         OutputStreamResult(OutputStreamInterface& _this) : stream(_this) {}
      };
      auto GetWriteResult() { return OutputStreamResult(*this); }
      ///////////////////////////////////////////////////////////////////////////////

   };

   class SizingStream : public OutputStreamInterface
   {
   public:
      virtual size_t WriteImpl(const uint8_t*, size_t length) override { return length; }
   };

   class StringOutputStream : public OutputStreamInterface
   {
      std::string  m_output;
      size_t       m_maxCapacity;  // max we send in one go

   public:
      StringOutputStream(size_t maxCapacity = 0xFFFF, size_t initialBufferSize = 0);
      virtual size_t WriteImpl(const uint8_t* data, size_t length) override;
      const std::string& GetString() { return m_output; }
      void Clear() { m_output = ""; }
   };

   class FixedBufferOutputStream : public OutputStreamInterface
   {
      uint8_t  *m_output;
      size_t    m_size;
      const size_t    m_maxCapacity;  // max we send in one go

   public:
      FixedBufferOutputStream(char    *buffer, size_t maxCapacity);
      FixedBufferOutputStream(uint8_t *buffer, size_t maxCapacity);

   protected:
      virtual size_t WriteImpl(const uint8_t* data, size_t length) override;
   };

   class StringInputStream : public RomInputStream
   {
      std::string m_input;

   public:
      StringInputStream(std::string&& input)
         : RomInputStream(input.data(), input.length())
         , m_input(std::move(input))
      {}
   };

   class StreamInterface : public InputStreamInterface, public OutputStreamInterface
   {};

   using EmbeddedJSONWriter = std::function< void(jude::OutputStreamInterface&) >;
}
