#include <gtest/gtest.h>
#include <vector>
#include <jude/core/cpp/Stream.h>
#include "core/test_base.h"

using namespace std;

class TestOutputStream : public JudeTestBase
{
public:

   void CheckNonBufferedOutput(const char *dataToWrite, size_t length, const char *expected)
   {
      std::stringstream ss;
      jude::OutputStreamWrapper mockOutput(ss, 0);
      jude_ostream_t *ostream = &mockOutput.m_ostream;

      auto result = jude_ostream_write(ostream, reinterpret_cast<const uint8_t*>(dataToWrite), length);
      ASSERT_EQ(result, length);
      ASSERT_FALSE(ostream->has_error) << " Failed writing: " << dataToWrite;
      ASSERT_EQ(strlen(expected), ss.str().length()) << " Length error checking: " << dataToWrite;
      std::string actual = ss.str();
      ASSERT_STREQ(expected, actual.c_str());
   }

   void CheckBufferedOutput(const char *dataToWrite, size_t length, size_t bufferSize, const char *expectedOutput, const char *expectedBuffer)
   {
      std::stringstream ss;
      jude::OutputStreamWrapper mockOutput(ss, bufferSize);
      jude_ostream_t *ostream = &mockOutput.m_ostream;

      auto result = jude_ostream_write(ostream, reinterpret_cast<const uint8_t*>(dataToWrite), length);
      ASSERT_EQ(result, length);
      
      ASSERT_FALSE(ostream->has_error) << " Failed writing: " << dataToWrite;
      ASSERT_EQ(strlen(expectedOutput), ss.str().length()) << " Length error checking: " << dataToWrite;
      
      std::string buffer(ostream->buffer.m_data, ostream->buffer.m_data + ostream->buffer.m_size);
      ASSERT_STREQ(expectedBuffer, buffer.c_str());
      
      std::string actual = ss.str();
      ASSERT_STREQ(expectedOutput, actual.c_str());

      // now flush
      ASSERT_TRUE(jude_ostream_flush(ostream));
      std::string actualFlushedOutput = ss.str();
      std::string expectedFlushedOutput = string(expectedOutput) + string(expectedBuffer);
      ASSERT_STREQ(expectedFlushedOutput.c_str(), actualFlushedOutput.c_str());
   }
};

TEST_F(TestOutputStream, NonBufferedOutput)
{
   CheckNonBufferedOutput("Hello", 1, "H");
   CheckNonBufferedOutput("Hello", 3, "Hel");
   CheckNonBufferedOutput("Hello", 5, "Hello");
}

TEST_F(TestOutputStream, BufferedOutput)
{
   //                 | InputData       | Length To write | Buffer Size | Expected Output | Expected left in buffer
   CheckBufferedOutput("Hello, everyone",                1,            1,               "", "H");
   CheckBufferedOutput("Hello, everyone",                3,            1,             "He", "l");
   CheckBufferedOutput("Hello, everyone",                5,            4,           "Hell", "o");
   CheckBufferedOutput("Hello, everyone",               32,           32,               "", "Hello, everyone");
   CheckBufferedOutput("Hello, everyone",               15,            1, "Hello, everyon", "e");
}
