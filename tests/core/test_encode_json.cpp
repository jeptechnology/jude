#include <gtest/gtest.h>

#include "jude/jude.h"
#include "jude/core/cpp/Stream.h"
#include "test_base.h"

using namespace std;

class JSON_EncodeTests : public JudeTestBase
{
public:

   JSON_EncodeTests()
   {
      jude_object_clear_all(optionals_object);
      jude_object_clear_all(repeats_object);
   }

   void CheckJsonCreatedForSingleFields(string json)
   {
      std::stringstream ss;
      jude::OutputStreamWrapper mockOutput(ss);
      mockOutput.m_ostream.output_recently_cleared_as_null = true;
      bool result = jude_encode(&mockOutput.m_ostream, optionals_object);
      jude_ostream_flush(&mockOutput.m_ostream);
      ASSERT_TRUE(result) << "FAIL: " << jude_ostream_get_error(&mockOutput.m_ostream) << " in JSON: " << json;
      ASSERT_STREQ(json.c_str(), ss.str().c_str());
   }

   void CheckJsonCreatedForArrayFields(string json)
   {
      std::stringstream ss;
      jude::OutputStreamWrapper mockOutput(ss);
      mockOutput.m_ostream.output_recently_cleared_as_null = true;
      bool result = jude_encode(&mockOutput.m_ostream, repeats_object);
      jude_ostream_flush(&mockOutput.m_ostream);
      ASSERT_TRUE(result) << "FAIL: " << jude_ostream_get_error(&mockOutput.m_ostream) << " in JSON: " << json;
      ASSERT_STREQ(json.c_str(), ss.str().c_str());
   }
};

TEST_F(JSON_EncodeTests, empty)
{
   CheckJsonCreatedForSingleFields("{}");
   CheckJsonCreatedForArrayFields("{}");
}

#define CHECK_SINGLE_FIELD(field, value, expected)                    \
   CheckJsonCreatedForSingleFields("{}");                             \
   optionals.Set_ ## field(value);                                    \
   CheckJsonCreatedForSingleFields("{\"" #field "\":" #expected "}"); \
   optionals.Clear_ ## field();                                       \
   CheckJsonCreatedForSingleFields("{\"" #field "\":null}");          \
   optionals.ClearChangeMarkers();                                    \
   CheckJsonCreatedForSingleFields("{}");                             \

#define CHECK_ARRAY_FIELD(field, type, expected, ...) do {           \
   type values[] = { __VA_ARGS__ };                                  \
   size_t elementCount = sizeof(values) / sizeof(values[0]);         \
   repeats.Get_ ## field ## s().clear();                             \
   for (unsigned index = 0; index < elementCount; index++)           \
   {                                                                 \
      repeats.Get_ ## field ## s().Add(values[index]);               \
   }                                                                 \
   CheckJsonCreatedForArrayFields("{\"" #field "\":" expected "}");  \
   repeats.Get_ ## field ## s().clear();                             \
   CheckJsonCreatedForArrayFields("{\"" #field "\":null}");          \
   } while(false)

TEST_F(JSON_EncodeTests, int8_type)
{
   CHECK_SINGLE_FIELD(int8_type, 0, 0);
   CHECK_SINGLE_FIELD(int8_type, 5, 5);
   CHECK_SINGLE_FIELD(int8_type, -5, -5);
   CHECK_SINGLE_FIELD(int8_type, INT8_MIN, -128);
   CHECK_SINGLE_FIELD(int8_type, INT8_MAX, 127);
}

TEST_F(JSON_EncodeTests, int16_type)
{
   CHECK_SINGLE_FIELD(int16_type, 0, 0);
   CHECK_SINGLE_FIELD(int16_type, 5, 5);
   CHECK_SINGLE_FIELD(int16_type, -5, -5);
   CHECK_SINGLE_FIELD(int16_type, INT16_MIN, -32768);
   CHECK_SINGLE_FIELD(int16_type, INT16_MAX, 32767);
}

TEST_F(JSON_EncodeTests, int32_type)
{
   CHECK_SINGLE_FIELD(int32_type, 0, 0);
   CHECK_SINGLE_FIELD(int32_type, 5, 5);
   CHECK_SINGLE_FIELD(int32_type, -5, -5);
   CHECK_SINGLE_FIELD(int32_type, INT32_MIN, -2147483648);
   CHECK_SINGLE_FIELD(int32_type, INT32_MAX, 2147483647);
}

TEST_F(JSON_EncodeTests, int64_type)
{
   CHECK_SINGLE_FIELD(int64_type, 0, 0);
   CHECK_SINGLE_FIELD(int64_type, 5, 5);
   CHECK_SINGLE_FIELD(int64_type, -5, -5);
   CHECK_SINGLE_FIELD(int64_type, INT64_MIN, -9223372036854775808);
   CHECK_SINGLE_FIELD(int64_type, INT64_MAX, 9223372036854775807);
}

TEST_F(JSON_EncodeTests, uint8_type)
{
   CHECK_SINGLE_FIELD(uint8_type, 0, 0);
   CHECK_SINGLE_FIELD(uint8_type, 5, 5);
   CHECK_SINGLE_FIELD(uint8_type, UINT8_MAX, 255);
}

TEST_F(JSON_EncodeTests, uint16_type)
{
   CHECK_SINGLE_FIELD(uint16_type, 0, 0);
   CHECK_SINGLE_FIELD(uint16_type, 5, 5);
   CHECK_SINGLE_FIELD(uint16_type, UINT16_MAX, 65535);
}

TEST_F(JSON_EncodeTests, uint32_type)
{
   CHECK_SINGLE_FIELD(uint32_type, 0, 0);
   CHECK_SINGLE_FIELD(uint32_type, 5, 5);
   CHECK_SINGLE_FIELD(uint32_type, UINT32_MAX, 4294967295);
}

TEST_F(JSON_EncodeTests, uint64_type)
{
   CHECK_SINGLE_FIELD(uint64_type, 0, 0);
   CHECK_SINGLE_FIELD(uint64_type, 5, 5);
   CHECK_SINGLE_FIELD(uint64_type, UINT64_MAX, 18446744073709551615);
}

TEST_F(JSON_EncodeTests, bool_type)
{
   CHECK_SINGLE_FIELD(bool_type, 0, false);
   CHECK_SINGLE_FIELD(bool_type, 1, true);
   CHECK_SINGLE_FIELD(bool_type, false, false);
   CHECK_SINGLE_FIELD(bool_type, true, true);
}

TEST_F(JSON_EncodeTests, string_type)
{
   CHECK_SINGLE_FIELD(string_type, "", "");
   CHECK_SINGLE_FIELD(string_type, "Hello", "Hello");
   CHECK_SINGLE_FIELD(string_type, "\"", "\"");
}

TEST_F(JSON_EncodeTests, int8_type_array)
{
   CHECK_ARRAY_FIELD(int8_type, int8_t, "[0]",        0);
   CHECK_ARRAY_FIELD(int8_type, int8_t, "[0,1]",      0, 1);
   CHECK_ARRAY_FIELD(int8_type, int8_t, "[5,-5]",     5, -5);
   CHECK_ARRAY_FIELD(int8_type, int8_t, "[-128,127]", INT8_MIN, INT8_MAX);
}

TEST_F(JSON_EncodeTests, int16_type_array)
{
   CHECK_ARRAY_FIELD(int16_type, int16_t, "[0]", 0);
   CHECK_ARRAY_FIELD(int16_type, int16_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(int16_type, int16_t, "[5,-5]", 5, -5);
   CHECK_ARRAY_FIELD(int16_type, int16_t, "[-32768,32767]", INT16_MIN, INT16_MAX);
}

TEST_F(JSON_EncodeTests, int32_type_array)
{
   CHECK_ARRAY_FIELD(int32_type, int32_t, "[0]", 0);
   CHECK_ARRAY_FIELD(int32_type, int32_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(int32_type, int32_t, "[5,-5]", 5, -5);
   CHECK_ARRAY_FIELD(int32_type, int32_t, "[-2147483648,2147483647]", INT32_MIN, INT32_MAX);
}

TEST_F(JSON_EncodeTests, int64_type_array)
{
   CHECK_ARRAY_FIELD(int64_type, int64_t, "[0]", 0);
   CHECK_ARRAY_FIELD(int64_type, int64_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(int64_type, int64_t, "[5,-5]", 5, -5);
   CHECK_ARRAY_FIELD(int64_type, int64_t, "[-9223372036854775808,9223372036854775807]", INT64_MIN, INT64_MAX);
}

TEST_F(JSON_EncodeTests, uint8_type_array)
{
   CHECK_ARRAY_FIELD(uint8_type, uint8_t, "[0]", 0);
   CHECK_ARRAY_FIELD(uint8_type, uint8_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(uint8_type, uint8_t, "[0,1,2,255]", 0, 1, 2, UINT8_MAX);
}

TEST_F(JSON_EncodeTests, uint16_type_array)
{
   CHECK_ARRAY_FIELD(uint16_type, uint16_t, "[0]", 0);
   CHECK_ARRAY_FIELD(uint16_type, uint16_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(uint16_type, uint16_t, "[0,1,2,65535]", 0, 1, 2, UINT16_MAX);
}

TEST_F(JSON_EncodeTests, uint32_type_array)
{
   CHECK_ARRAY_FIELD(uint32_type, uint32_t, "[0]", 0);
   CHECK_ARRAY_FIELD(uint32_type, uint32_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(uint32_type, uint32_t, "[0,1,2,4294967295]", 0, 1, 2, UINT32_MAX);
}

TEST_F(JSON_EncodeTests, uint64_type_array)
{
   CHECK_ARRAY_FIELD(uint64_type, uint64_t, "[0]", 0);
   CHECK_ARRAY_FIELD(uint64_type, uint64_t, "[0,1]", 0, 1);
   CHECK_ARRAY_FIELD(uint64_type, uint64_t, "[0,1,2,18446744073709551615]", 0, 1, 2, UINT64_MAX);
}

TEST_F(JSON_EncodeTests, bool_type_array)
{
   CHECK_ARRAY_FIELD(bool_type, bool, "[false]", 0);
   CHECK_ARRAY_FIELD(bool_type, bool, "[false,true]", 0, 1);
   CHECK_ARRAY_FIELD(bool_type, bool, "[true,false,true]", true, false, true);
   CHECK_ARRAY_FIELD(bool_type, bool, "[true,true,false,false]", 1, true, 0, false);
}

TEST_F(JSON_EncodeTests, string_type_array)
{
   CHECK_ARRAY_FIELD(string_type, const char*, R"([""])", "");
   CHECK_ARRAY_FIELD(string_type, const char*, R"(["","Hello"])", "", "Hello");
   CHECK_ARRAY_FIELD(string_type, const char*, R"(["Hello","World"])", "Hello", "World");
}
