#pragma once

#include "jude/jude.h"
#include "jude/core/cpp/Stream.h"

#include <vector>

using namespace std;

class MockInputStream : public jude::InputStreamInterface
{
   char              m_buffer[128];
public:
   std::vector<char> data{};                 // underlying data
   size_t            maxChunkSize{ 0xffff }; // max we send in one go
   bool              inject_error{ false };  // if set to true, we return an error
   size_t            numberOfReads{ 0 };

   MockInputStream()
   {
      SetInputBuffer(m_buffer, sizeof(m_buffer));
   }

   void SetData(const std::string& text)
   {
      data.clear();
      std::copy(text.begin(), text.end(), std::back_inserter(data));
   }

   size_t ReadImpl(uint8_t *buffer, size_t maxBytesToRead) override
   {
      numberOfReads++;

      if (inject_error)
      {
         return 0;
      }

      // bytesToRead limited by: data left in buffer, max size requested, max chunk size
      size_t bytesToRead = data.size();
      if (maxBytesToRead < bytesToRead)
      {
         bytesToRead = maxBytesToRead;
      }
      if (maxChunkSize < bytesToRead)
      {
         bytesToRead = maxChunkSize;
      }

      memcpy(buffer, data.data(), bytesToRead);
      data.erase(data.begin(), data.begin() + bytesToRead);

      return bytesToRead;
   }
};
