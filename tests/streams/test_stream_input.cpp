#include <gtest/gtest.h>
#include <vector>
#include "streams/mock_istream.h"
#include "core/test_base.h"

class TestInputStream : public JudeTestBase
{
public:
   MockInputStream mockInput;
   jude_istream_t *istream = mockInput.GetLowLevelInputStream();
};

TEST_F(TestInputStream, read_on_empty_stream_results_in_end_of_file)
{
   uint8_t c;
   mockInput.SetData("");

   auto result = jude_istream_read(istream, &c, 1);

   EXPECT_EQ(0, result);
   EXPECT_FALSE(istream->has_error);
   EXPECT_TRUE(jude_istream_is_eof(istream));
}

TEST_F(TestInputStream, reading_all_bytes_of_non_empty_stream_results_in_end_of_file)
{
   uint8_t c;
   mockInput.SetData("123");   
   
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
   mockInput.SetData("123");

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
   mockInput.SetData("Hello");

   jude_istream_read(istream, &c, 1);
   
   EXPECT_FALSE(jude_istream_is_eof(istream));
   EXPECT_FALSE(istream->has_error);
   EXPECT_EQ('H', c);
}

TEST_F(TestInputStream, successfully_reading_uint8_t_results_in_success)
{
   uint8_t c;
   mockInput.SetData("Hello");

   jude_istream_read(istream, &c, 1);

   EXPECT_EQ('H', c);
}

TEST_F(TestInputStream, readimpl_failure_returns_0_result)
{
   uint8_t c;
   mockInput.inject_error = true;

   auto result = jude_istream_read(istream, &c, 1);

   EXPECT_EQ(0, result);
   EXPECT_TRUE(jude_istream_is_eof(istream));
}

TEST_F(TestInputStream, readimpl_failure_asserts_EOF_even_if_there_was_data_to_read)
{
   uint8_t buffer[32];
   mockInput.inject_error = true;
   mockInput.SetData("Hello");

   auto result = jude_istream_read(istream, buffer, sizeof(buffer));

   EXPECT_EQ(0, result);
   EXPECT_TRUE(jude_istream_is_eof(istream));
}

TEST_F(TestInputStream, successfully_reading_buffer_results_in_success)
{
   uint8_t buffer[3];
   mockInput.SetData("Hello");

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

/*
TEST_F(TestInputStream, nonblocking_read_of_empty_stream_results_in_success_with_zero_bytes_read_and_no_eof)
{
   uint8_t buffer[32];
   mockInput.SetData("");
   mockInput.markEofWhenNoMoreBytes = false;

   auto result = jude_istream_read(istream, buffer, sizeof(buffer));
   
   EXPECT_TRUE(result);
   EXPECT_EQ(0, *result);
   EXPECT_FALSE(mockInput.IsEof());
}

TEST_F(TestInputStream, blocking_read_of_empty_stream_results_in_success_with_zero_bytes_read_and_eof)
{
   uint8_t buffer[32];
   mockInput.SetData("");
   mockInput.markEofWhenNoMoreBytes = true;

   auto result = jude_istream_read(istream, buffer, sizeof(buffer));
   
   EXPECT_TRUE(result);
   EXPECT_EQ(0, *result);
   EXPECT_TRUE(mockInput.IsEof());
}

TEST_F(TestInputStream, read_line_of_empty_stream_results_in_success_with_zero_bytes_read_and_eof)
{
   std::string line;
   mockInput.SetData("");
   
   auto result = mockInput.ReadLine(line);
   
   EXPECT_TRUE(result);
   EXPECT_EQ(0, *result);
   EXPECT_TRUE(mockInput.IsEof());
}

TEST_F(TestInputStream, read_line_of_single_line_stream)
{
   std::string line;
   const char data[] = "A single line";
   mockInput.SetData(data);
   
   auto result = mockInput.ReadLine(line);
   
   EXPECT_TRUE(result);
   EXPECT_EQ(strlen(data), *result);
   EXPECT_STREQ(data, line.c_str());
   EXPECT_FALSE(mockInput.IsEof()); // not EOF yet as last call got a line...

   mockInput.ReadLine(line); // try once more
   EXPECT_TRUE(mockInput.IsEof()); // now we are EOF!
}

TEST_F(TestInputStream, read_line_of_multi_line_stream_results_in_success_and_no_eof)
{
   std::string line1, line2;
   const char data[] = "Line one\r\n"
                       "--Line Two--";
   mockInput.SetData(data);
   
   auto result = mockInput.ReadLineN(32, line1);
   
   EXPECT_TRUE(result);
   EXPECT_EQ(strlen("Line one\r\n"), *result); // we read up to \n (including the \r)
   EXPECT_STREQ("Line one", line1.c_str());  // but we trimmed the string (i.e. no \r or \n in line1)
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLineN(32, line2);

   EXPECT_TRUE(result);
   EXPECT_STREQ("--Line Two--", line2.c_str());
   EXPECT_FALSE(mockInput.IsEof());
}

TEST_F(TestInputStream, read_line_with_delimiter_reads_truncated_line)
{
   std::string line1, line2;
   const char data[] = "Line one#--Line Two--##";
   mockInput.SetData(data);

   auto result = mockInput.ReadLineN(32, line1, '#');

   EXPECT_TRUE(result);
   EXPECT_EQ(strlen("Line one#"), *result);
   EXPECT_STREQ("Line one", line1.c_str());
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLineN(32, line2, '#');

   EXPECT_TRUE(result);
   EXPECT_STREQ("--Line Two--", line2.c_str());
   EXPECT_FALSE(mockInput.IsEof());
}

TEST_F(TestInputStream, read_line_with_length_restriction_truncates_line)
{
   std::string line1, line2;
   const char data[] = "A very long line of text\r\n"
                       "--Another line--";
   mockInput.SetData(data);

   auto result = mockInput.ReadLineN(16, line1);

   EXPECT_TRUE(result);
   EXPECT_EQ(16, *result);
   EXPECT_STREQ("A very long line", line1.c_str());
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLineN(16, line2);

   EXPECT_TRUE(result);
   EXPECT_STREQ(" of text", line2.c_str());
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLineN(16, line2);

   EXPECT_TRUE(result);
   EXPECT_STREQ("--Another line--", line2.c_str());
   EXPECT_FALSE(mockInput.IsEof());
}

TEST_F(TestInputStream, read_line_with_length_restriction_and_delimieter_truncates_line)
{
   std::string line1, line2;
   const char data[] = "A very long line of text#"
                       "--Another line--";
   mockInput.SetData(data);

   auto result = mockInput.ReadLineN(16, line1, '#');

   EXPECT_TRUE(result);
   EXPECT_EQ(16, *result);
   EXPECT_STREQ("A very long line", line1.c_str());
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLineN(16, line2, '#');

   EXPECT_TRUE(result);
   EXPECT_STREQ(" of text", line2.c_str());
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLineN(16, line2, '#');

   EXPECT_TRUE(result);
   EXPECT_STREQ("--Another line--", line2.c_str());
   EXPECT_FALSE(mockInput.IsEof());
}

TEST_F(TestInputStream, read_line_keeps_reading_empty_lines_until_eof)
{
   std::string line;
   const char data[] = "\r\n\r\n\n\n"; // 2 x CRLF, followed by 2 x LF
   mockInput.SetData(data);

   auto result = mockInput.ReadLine(line, 32);  // CRLF

   EXPECT_TRUE(result);
   EXPECT_STREQ("", line.c_str());
   EXPECT_FALSE(mockInput.IsEof());

   mockInput.ReadLine(line, 32); // CRLF

   EXPECT_TRUE(result);
   EXPECT_STREQ("", line.c_str());
   EXPECT_FALSE(mockInput.IsEof());
  
   mockInput.ReadLine(line, 32); // LF

   EXPECT_TRUE(result);
   EXPECT_STREQ("", line.c_str());
   EXPECT_FALSE(mockInput.IsEof());
   
   mockInput.ReadLine(line, 32); // LF

   EXPECT_TRUE(result);
   EXPECT_STREQ("", line.c_str());
   EXPECT_FALSE(mockInput.IsEof());
   
   mockInput.ReadLine(line, 32); // Should be EOF now!

   EXPECT_TRUE(result);
   EXPECT_STREQ("", line.c_str());
   EXPECT_TRUE(mockInput.IsEof());
}
*/
