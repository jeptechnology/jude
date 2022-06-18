#include <gtest/gtest.h>
#include <vector>
#include <sstream>
#include <jude/core/cpp/Stream.h>
#include "core/test_base.h"

using namespace jude;

class TestInputStream : public JudeTestBase
{
public:
   std::stringstream ss;
   InputStreamWrapper mockInput { ss };
   jude_istream_t *istream = &mockInput.m_istream;
};

TEST_F(TestInputStream, read_on_empty_stream_results_in_end_of_file)
{
   uint8_t c;
   ss.str("");

   auto result = jude_istream_read(istream, &c, 1);

   EXPECT_EQ(0, result);
   EXPECT_FALSE(istream->has_error);
   EXPECT_TRUE(jude_istream_is_eof(istream));
}

TEST_F(TestInputStream, reading_all_bytes_of_non_empty_stream_results_in_end_of_file)
{
   uint8_t c;
   ss.str("123");   
   
   // Read first byte
   auto result = jude_istream_read(istream, &c, 1);

   EXPECT_EQ(1, result);
   EXPECT_FALSE(jude_istream_is_eof(istream));
   
   // read second byte
   result = jude_istream_read(istream, &c, 1);
   EXPECT_EQ(1, result);
   EXPECT_FALSE(jude_istream_is_eof(istream));

   // read last byte
   result = jude_istream_read(istream, &c, 1);
   EXPECT_EQ(1, result);
   EXPECT_FALSE(jude_istream_is_eof(istream));

   // attempt to read past last byte
   result = jude_istream_read(istream, &c, 1);
   EXPECT_EQ(0, result);
   EXPECT_TRUE(jude_istream_is_eof(istream));
   EXPECT_FALSE(istream->has_error);
}

TEST_F(TestInputStream, reading_all_bytes_of_non_empty_stream_in_one_go_results_in_end_of_file_on_subsequent_read)
{
   uint8_t buffer[32];
   ss.str("123");

   // When reading into a buffer with more capacity that there are bytes to read in stream
   auto result = jude_istream_read(istream, buffer, sizeof(buffer));

   // Then we read as much as we could (3 bytes) and no EOF declared yet
   EXPECT_EQ(3, result);
   EXPECT_FALSE(jude_istream_is_eof(istream));

   // When we attempt another read of up to 32 bytes...
   result = jude_istream_read(istream, buffer, sizeof(buffer));

   // Then we read no more bytes and EOF declared
   EXPECT_EQ(0, result); // no error - just EOF
   EXPECT_TRUE(jude_istream_is_eof(istream));
   EXPECT_FALSE(istream->has_error);
}

TEST_F(TestInputStream, successfully_reading_char_results_in_success)
{
   uint8_t c;
   ss.str("Hello");

   jude_istream_read(istream, &c, 1);
   
   EXPECT_FALSE(jude_istream_is_eof(istream));
   EXPECT_FALSE(istream->has_error);
   EXPECT_EQ('H', c);
}

TEST_F(TestInputStream, successfully_reading_uint8_t_results_in_success)
{
   uint8_t c;
   ss.str("Hello");

   jude_istream_read(istream, &c, 1);

   EXPECT_EQ('H', c);
}

TEST_F(TestInputStream, readimpl_failure_returns_0_result)
{
   uint8_t c;
   ss.str("");

   auto result = jude_istream_read(istream, &c, 1);

   EXPECT_EQ(0, result);
   EXPECT_TRUE(jude_istream_is_eof(istream));
}

TEST_F(TestInputStream, readimpl_failure_asserts_EOF_even_if_there_was_data_to_read)
{
   uint8_t buffer[32];
   ss.str("");

   auto result = jude_istream_read(istream, buffer, sizeof(buffer));

   EXPECT_EQ(0, result);
   EXPECT_TRUE(jude_istream_is_eof(istream));
}

TEST_F(TestInputStream, successfully_reading_buffer_results_in_success)
{
   uint8_t buffer[3];
   ss.str("Hello");

   auto result = jude_istream_read(istream, buffer, sizeof(buffer));

   EXPECT_FALSE(istream->has_error);
   EXPECT_EQ(3, result); // we read 3 bytes
   EXPECT_EQ('H', buffer[0]);
   EXPECT_EQ('e', buffer[1]);
   EXPECT_EQ('l', buffer[2]);

   auto result2 = jude_istream_read(istream, buffer, sizeof(buffer));

   EXPECT_FALSE(istream->has_error);
   EXPECT_EQ(2, result2); // we have now read remaining 2 bytes
   EXPECT_EQ('l', buffer[0]);
   EXPECT_EQ('o', buffer[1]);
}
