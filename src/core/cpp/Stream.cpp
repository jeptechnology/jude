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

namespace jude 
{
   size_t InputStreamInterface::ReadCallback(void* user_data, uint8_t* data, size_t length)
   {
      auto _this = static_cast<InputStreamInterface*>(user_data);
      return _this->Read(data, length);
   }

   InputStreamInterface::InputStreamInterface(const jude_decode_transport_t* transport)
   {
      jude_istream_create(
         &m_istream,
         transport,
         ReadCallback,
         this,
         nullptr,
         0);
   }

   // Use this to allow input stream to be buffered using the given buffer
   void InputStreamInterface::SetInputBuffer(char* buffer, size_t bufferLen)
   {
      jude_istream_create(
         &m_istream,
         m_istream.transport,
         ReadCallback,
         this,
         buffer,
         bufferLen);
   }

   void InputStreamInterface::ClearBuffer()
   {
      m_istream.buffer.m_readIndex = 0;
   }

   // Use this to set the type of encoding that you want - e.g. JSON, protobuf, etc
   void InputStreamInterface::SetInputDecoding(const jude_decode_transport_t* transport)
   {
      jude_istream_create(
         &m_istream,
         transport,
         ReadCallback,
         this,
         (char*)m_istream.buffer.m_data,
         m_istream.buffer.m_capacity);
   }

   size_t InputStreamInterface::Read(uint8_t* data, size_t length)
   {
      m_lastBytesRead = ReadImpl(data, length);
      if (m_lastBytesRead <= length)
      {
         m_totalBytesRead += m_lastBytesRead;
         return m_lastBytesRead;
      }
      return 0;
   }

   size_t InputStreamInterface::Read(std::vector<uint8_t>& destination)
   {
      auto result = Read(destination.data(), destination.capacity());
      if (result >= 0 && result <= destination.capacity())
      {
         destination.resize(result);
      }
      return result;
   }

   size_t InputStreamInterface::ReadLine(std::string& line, char delimiter)
   {
      constexpr size_t DefaultMaxBytesPerLine = 1024;
      return ReadLineN(DefaultMaxBytesPerLine, line, delimiter);
   }

   size_t InputStreamInterface::ReadLineN(size_t maxBytesToRead, std::string& line, char delimiter)
   {
      line.clear();

      if (HasInputError())
      {
         return 0;
      }

      size_t totalBytesRead = 0;
      uint8_t byte;

      while (maxBytesToRead > 0) // always blocking read for read line
      {
         auto result = Read(&byte, 1);
         if (HasInputError())
         {
            return 0;
         }

         if (result == 0)
         {
            break;
         }

         totalBytesRead += result;

         if (  (byte == '\n')
            || (delimiter == byte))
         {
            break;
         }

         if (byte != '\r')
         {
            line += byte;
            maxBytesToRead--;
         }
      }

      return totalBytesRead;
   }

   size_t OutputStreamInterface::Pipe(InputStreamInterface &inputStream, size_t chunkSizeBytes, std::optional<size_t> expectedInputSize, std::function<void(size_t)> progressCallback)
   {
      // If we are already in error state then short cut return
      if (HasOutputError())
      {
         return 0;
      }

      if (!inputStream)
      {
         // transfer the error to our output stream
         SetOutputErrorMsg(inputStream.GetInputErrorMsg());
         return 0;
      }

      size_t totalBytesWritten = 0;
      std::vector<uint8_t> buffer(chunkSizeBytes);
      
      while (true)
      {
         size_t bytesToReadThisTime = chunkSizeBytes;
         if (expectedInputSize && *expectedInputSize < chunkSizeBytes)
         {
            bytesToReadThisTime = *expectedInputSize;
         }

         auto numberOfBytesJustRead = inputStream.Read(buffer.data(), bytesToReadThisTime);
         if (!inputStream)
         {
            // transfer the error to our output stream
            SetOutputErrorMsg(inputStream.GetInputErrorMsg());
            return 0;
         }

         // inside here we know the read was successful...
         if (numberOfBytesJustRead == 0)
         {
            break; // eof of input stream
         }

         auto bytesJustWritten = Write(buffer.data(), numberOfBytesJustRead);
         totalBytesWritten += bytesJustWritten;
         if (HasOutputError())
         {
            break;
         }

         if (numberOfBytesJustRead != bytesJustWritten)
         {
            SetOutputErrorMsg("ERROR: Piping has been blocked by output stream");
            break;
         }

         totalBytesWritten += bytesJustWritten;
         if (progressCallback)
         {
            progressCallback(totalBytesWritten);
         }

         if (expectedInputSize && totalBytesWritten >= *expectedInputSize)
         {
            break; // we are done!
         }
      }

      // if there was any issue with the input stream then transfer that problem to this output stream
      if (!inputStream)
      {
         // transfer the error to our output stream
         SetOutputErrorMsg(inputStream.GetInputErrorMsg());
      }

      return totalBytesWritten;
   }


   RomInputStream::RomInputStream(const char* data)
   {
      if (data)
      {
         SetConstData(data);
      }
   }

   RomInputStream::RomInputStream(const char* data, size_t length)
   {
      SetConstData(data, length);
   }

   RomInputStream::RomInputStream(const uint8_t* data, size_t length)
   {
      SetConstData(data, length);
   }
   
   void RomInputStream::SetConstData(const uint8_t *data, size_t length)
   {
      m_data = data;
      m_bytesLeft = length;
      ClearBuffer();
      SetInputBuffer(m_buffer, sizeof(m_buffer));
   }

   size_t RomInputStream::ReadImpl(uint8_t* data, size_t length)
   {
      size_t bytesToRead = m_bytesLeft > length ? length : m_bytesLeft;
      memcpy(data, m_data, bytesToRead);
      m_data += bytesToRead;
      m_bytesLeft -= bytesToRead;
      return bytesToRead;
   }

   size_t OutputStreamInterface::WriteCallback(void* user_data, const uint8_t* data, size_t length)
   {
      auto _this = static_cast<OutputStreamInterface*>(user_data);
      return _this->WriteImpl(data, length);
   }

   OutputStreamInterface::OutputStreamInterface(const jude_encode_transport_t* transport)
   {
      jude_ostream_create(
         &m_ostream,
         transport,
         WriteCallback,
         this,
         nullptr,
         0);
   }

   // Use this to allow output stream to be buffered using the given buffer
   void OutputStreamInterface::SetOutputBuffer(char* buffer, size_t bufferLen)
   {
      jude_ostream_create(
         &m_ostream,
         m_ostream.transport,
         WriteCallback,
         this,
         buffer,
         bufferLen);
   }

   // Use this to set the type of encoding that you want - e.g. JSON, protobuf, etc
   void OutputStreamInterface::SetOutputEncoding(const jude_encode_transport_t* transport)
   {
      jude_ostream_create(
         &m_ostream,
         transport,
         WriteCallback,
         this,
         (char *)m_ostream.buffer.m_data,
         m_ostream.buffer.m_capacity);
   }

   size_t OutputStreamInterface::Print(const char* text)
   {
      return Write(reinterpret_cast<const uint8_t *>(text), strlen(text));
   }

   size_t OutputStreamInterface::Printf(size_t max, const char* format, ...)
   {
      constexpr size_t MaxLocalPrintfBuffer = 64;

      if (max <= MaxLocalPrintfBuffer)
      {
         // For speed, use stack buffer for small writes...
         char tmp[MaxLocalPrintfBuffer];
         va_list list;
         va_start(list, format);
         vsnprintf(tmp, max, format, list);
         va_end(list);
         
         return Print(tmp);
      }

      // larger string... use heap
      va_list list;
      va_start(list, format);
      auto length = vsnprintf(nullptr, 0, format, list);
      va_end(list);

      if (length > max)
      {
         length = max;
      }

      auto buffer = new char[length + 1];
      va_start(list, format);
      length = vsnprintf(buffer, length + 1, format, list);
      va_end(list);

      auto result = Print(buffer);
      delete[] buffer;

      return result;
   }

   bool OutputStreamInterface::Flush()
   {
      return jude_ostream_flush(&m_ostream);
   }

   size_t OutputStreamInterface::Write(const uint8_t* data, size_t length)
   {
      size_t bytesWritten = jude_ostream_write(&m_ostream, data, length);
      if (bytesWritten <= length)
      {
         return bytesWritten;
      }
      return 0;
   }

   StringOutputStream::StringOutputStream(size_t maxCapacity, size_t initialBufferSize)
      : m_maxCapacity(maxCapacity)
   {
      if (initialBufferSize)
      {
         m_output.reserve(initialBufferSize);
      }
   }

   size_t StringOutputStream::WriteImpl(const uint8_t* data, size_t length)
   {
      if (m_output.size() >= m_maxCapacity)
      {
         // overflow
         return 0;
      }

      if (length + m_output.size() > m_maxCapacity)
      {
         // truncate
         length = m_maxCapacity - m_output.size();
      }

      m_output.insert(m_output.end(), data, data + length);

      return length;
   }

   FixedBufferOutputStream::FixedBufferOutputStream(char* buffer, size_t maxCapacity)
      : m_output((uint8_t*)buffer)
      , m_size(0)
      , m_maxCapacity(maxCapacity)
   {}

   FixedBufferOutputStream::FixedBufferOutputStream(uint8_t* buffer, size_t maxCapacity)
      : m_output(buffer)
      , m_size(0)
      , m_maxCapacity(maxCapacity)
   {}

   size_t FixedBufferOutputStream::WriteImpl(const uint8_t* data, size_t length)
   {
      if (m_size >= m_maxCapacity)
      {
         // overflow
         return 0;
      }

      if (length + m_size > m_maxCapacity)
      {
         // truncate
         length = m_maxCapacity - m_size;
      }

      memcpy(&m_output[m_size], data, length);
      m_size += length;

      return length;
   }

}
