#pragma once

#include "jude/jude.h"
#include "jude/core/cpp/Stream.h"
#include <vector>

using namespace std;

class MockOutputStream : public jude::OutputStreamInterface
{
public:
   std::vector<char> output{};               // final output data
   std::vector<char> buffer{};               // buffer
   size_t            maxCapacity{ 0xffff };  // max we send in one go
   const char       *injectWriteError{ nullptr };  // non-null will mean we return an error on WriteImpl
   size_t            writeCalls{ 0 };

   MockOutputStream(size_t bufferSize = 0)
   {
      if (bufferSize)
      {
         buffer.reserve(bufferSize);
         this->SetOutputBuffer(buffer.data(), bufferSize);
      }
   }

   size_t WriteImpl(const uint8_t *data, size_t length)
   {
      writeCalls++;

      if (injectWriteError)
      {
         return 0;
      }

      if (output.size() >= maxCapacity)
      {
         return 0;
      }

      if (length + output.size() > maxCapacity)
      {
         length = maxCapacity - output.size();
      }

      output.insert(output.end(), data, data + length);

      return length;
   }

   std::string GetOutputString()
   {
      return std::string(output.data(), output.data() + output.size());
   }
};
