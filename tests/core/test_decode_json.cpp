#include <gtest/gtest.h>

#include "jude/jude.h"
#include "jude/core/cpp/Stream.h"
#include "test_base.h"

using namespace std;

class JSON_DecodeTests : public JudeTestBase
{
public:
   void CheckJsonParseOK(string json)
   {
      std::stringstream ss(json);
      jude::InputStreamWrapper mockInput(ss);
      bool result = jude_decode_noinit(&mockInput.m_istream, optionals_object);
      ASSERT_TRUE(result) << "FAIL: " << jude_istream_get_error(&mockInput.m_istream) << " in JSON: " << json;
   }

   void CheckJsonParseFail(string json, string expectedError)
   {
      std::stringstream ss(json);
      jude::InputStreamWrapper mockInput(ss);
      bool result = jude_decode_noinit(&mockInput.m_istream, optionals_object);
      ASSERT_FALSE(result) << "Unexpectedly succeeded to parse JSON: " << json;
      ASSERT_EQ(string(jude_istream_get_error(&mockInput.m_istream)), expectedError);
   }

   bool IsSet(jude_object_t* object, string name)
   {
      jude_iterator_t iter = jude_iterator_begin(object);
      if (!jude_iterator_find_by_name(&iter, name.c_str()))
      {
         return false;
      }
      return jude_iterator_is_touched(&iter);
   }

   bool IsSet(string name)
   {
      return IsSet(optionals_object, name);
   }
};

#define CHECK_SINGLE_FIELD(field, expected, value)                    \
   CheckJsonParseOK("{}");                                            \
   ASSERT_FALSE(IsSet(#field));                                       \
   CheckJsonParseOK("{\"" #field "\":" #value "}");                   \
   ASSERT_TRUE(IsSet(#field));                                        \
   ASSERT_EQ(optionals.Get_ ## field(), expected);                    \
   CheckJsonParseOK("{\"" #field "\":null}");                         \
   ASSERT_FALSE(IsSet(#field));                                       \

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


TEST_F(JSON_DecodeTests, read_unknown_tag_with_no_error)
{
   CheckJsonParseOK("{ \"Hello\": 1 }");
}

TEST_F(JSON_DecodeTests, read_empty_message_with_no_error)
{
   CheckJsonParseOK("{}");
   CheckJsonParseOK("{ }");
   CheckJsonParseOK(" \n{\n \r \t}\n");
}

TEST_F(JSON_DecodeTests, int8_type)
{
   CHECK_SINGLE_FIELD(int8_type, 0, 0);
   CHECK_SINGLE_FIELD(int8_type, 5, 5);
   CHECK_SINGLE_FIELD(int8_type, -5, -5);
   CHECK_SINGLE_FIELD(int8_type, INT8_MIN, -128);
   CHECK_SINGLE_FIELD(int8_type, INT8_MAX, 127);
}

TEST_F(JSON_DecodeTests, int16_type)
{
   CHECK_SINGLE_FIELD(int16_type, 0, 0);
   CHECK_SINGLE_FIELD(int16_type, 5, 5);
   CHECK_SINGLE_FIELD(int16_type, -5, -5);
   CHECK_SINGLE_FIELD(int16_type, INT16_MIN, -32768);
   CHECK_SINGLE_FIELD(int16_type, INT16_MAX, 32767);
}

TEST_F(JSON_DecodeTests, int32_type)
{
   CHECK_SINGLE_FIELD(int32_type, 0, 0);
   CHECK_SINGLE_FIELD(int32_type, 5, 5);
   CHECK_SINGLE_FIELD(int32_type, -5, -5);
   CHECK_SINGLE_FIELD(int32_type, INT32_MIN, -2147483648);
   CHECK_SINGLE_FIELD(int32_type, INT32_MAX, 2147483647);
}

TEST_F(JSON_DecodeTests, int64_type)
{
   CHECK_SINGLE_FIELD(int64_type, 0, 0);
   CHECK_SINGLE_FIELD(int64_type, 5, 5);
   CHECK_SINGLE_FIELD(int64_type, -5, -5);
   CHECK_SINGLE_FIELD(int64_type, INT64_MIN, -9223372036854775808);
   CHECK_SINGLE_FIELD(int64_type, INT64_MAX, 9223372036854775807);
}

TEST_F(JSON_DecodeTests, uint8_type)
{
   CHECK_SINGLE_FIELD(uint8_type, 0, 0);
   CHECK_SINGLE_FIELD(uint8_type, 5, 5);
   CHECK_SINGLE_FIELD(uint8_type, UINT8_MAX, 255);
}

TEST_F(JSON_DecodeTests, uint16_type)
{
   CHECK_SINGLE_FIELD(uint16_type, 0, 0);
   CHECK_SINGLE_FIELD(uint16_type, 5, 5);
   CHECK_SINGLE_FIELD(uint16_type, UINT16_MAX, 65535);
}

TEST_F(JSON_DecodeTests, uint32_type)
{
   CHECK_SINGLE_FIELD(uint32_type, 0, 0);
   CHECK_SINGLE_FIELD(uint32_type, 5, 5);
   CHECK_SINGLE_FIELD(uint32_type, UINT32_MAX, 4294967295);
}

TEST_F(JSON_DecodeTests, uint64_type)
{
   CHECK_SINGLE_FIELD(uint64_type, 0, 0);
   CHECK_SINGLE_FIELD(uint64_type, 5, 5);
   CHECK_SINGLE_FIELD(uint64_type, UINT64_MAX, 18446744073709551615);
}

TEST_F(JSON_DecodeTests, bool_type)
{
   CHECK_SINGLE_FIELD(bool_type, 0, false);
   CHECK_SINGLE_FIELD(bool_type, 1, true);
   CHECK_SINGLE_FIELD(bool_type, false, false);
   CHECK_SINGLE_FIELD(bool_type, true, true);
}

TEST_F(JSON_DecodeTests, string_type)
{
   CHECK_SINGLE_FIELD(string_type, "", "");
   CHECK_SINGLE_FIELD(string_type, "Hello", "Hello");
   CHECK_SINGLE_FIELD(string_type, "\"", "\"");
}

TEST_F(JSON_DecodeTests, read_a_known_tag_and_push_string)
{
   CheckJsonParseOK("{\"string_type\":\"TightSyntax\"}");

   EXPECT_TRUE(IsSet("string_type"));
   EXPECT_STREQ(optionals.Get_string_type().c_str(), "TightSyntax");

   CheckJsonParseOK("{  \"string_type\"  :  \"LooseSyntax\"  }");

   EXPECT_TRUE(IsSet("string_type"));
   EXPECT_STREQ(optionals.Get_string_type().c_str(), "LooseSyntax");

   CheckJsonParseOK("{\n\"string_type\"\n:\r\n\"NewlinesInSyntax\"\r\n}");

   EXPECT_TRUE(IsSet("string_type"));
   EXPECT_STREQ(optionals.Get_string_type().c_str(), "NewlinesInSyntax");
}

TEST_F(JSON_DecodeTests, read_a_known_tag_with_wrong_type)
{
   CheckJsonParseFail(
      "{\"string_type\": 190 }", 
      "string_type: Expecting one of n\"");
}

TEST_F(JSON_DecodeTests, read_a_known_tag_and_bad_string)
{
   CheckJsonParseFail(
      "{ \"string_type\" : \"NoEnclosingQuote... }", 
      "string_type: Unexpected EOF");

   CheckJsonParseFail(
      "{ \"string_type\" : NoOpeningQuote\" }",
      "string_type: Expecting one of n\"");
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_int32)
{
   CheckJsonParseOK("{\"int32_type\" : 30}");
   EXPECT_TRUE(IsSet("int32_type"));
   EXPECT_EQ(optionals.Get_int32_type(), 30);
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_sint64)
{
   CheckJsonParseOK("{\"int64_type\":-22}");
   EXPECT_TRUE(IsSet("int64_type"));
   EXPECT_EQ(optionals.Get_int64_type(), -22);
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_uint64_type)
{
   CheckJsonParseOK("{\"uint64_type\":43}");

   EXPECT_TRUE(IsSet("uint64_type"));
   EXPECT_EQ(optionals.Get_uint64_type(), 43);

   CheckJsonParseOK("{\"uint64_type\":  44  }");

   EXPECT_TRUE(IsSet("uint64_type"));
   EXPECT_EQ(optionals.Get_uint64_type(), 44);
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_bool_true)
{
   CheckJsonParseOK("{\"bool_type\":true}");

   EXPECT_TRUE(IsSet("bool_type"));
   EXPECT_EQ(optionals.Get_bool_type(), true);
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_bool_false)
{
   CheckJsonParseOK("{\"bool_type\":  false  }");

   EXPECT_TRUE(IsSet("bool_type"));
   EXPECT_EQ(optionals.Get_bool_type(), false);
}

TEST_F(JSON_DecodeTests, decode_multiple_tags_and_push_values)
{
   CheckJsonParseOK("{"
      "\"string_type\": \"Hello, World!\"  ,  "
      "\"UNKNOWN_TAG!\": { "
      "\"Hello\" : { "
      "\"Array\":\r\n [ 2, 2] "
      "}, "
      "\"Hello2\": \"a string that must be skipped\" "
      "},  "
      "\"uint64_type\":  203 ,  "
      "\"int32_type\": -3333 ,"
      "\"bool_type\":  true  "
      "}");

   EXPECT_TRUE(IsSet("string_type"));
   EXPECT_TRUE(IsSet("uint64_type"));
   EXPECT_TRUE(IsSet("int32_type"));
   EXPECT_TRUE(IsSet("bool_type"));
   EXPECT_STREQ(optionals.Get_string_type().c_str(), "Hello, World!");
   EXPECT_EQ(optionals.Get_uint64_type(), 203);
   EXPECT_EQ(optionals.Get_int32_type(), -3333);
   EXPECT_EQ(optionals.Get_bool_type(), true);
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_bytes)
{
   string helloWorldStr = "Hello, World!";   // Base64 encoded as below!
   CheckJsonParseOK("{\"bytes_type\": \"SGVsbG8sIFdvcmxkIQ==\" }");

   ASSERT_TRUE(IsSet("bytes_type"));
   ASSERT_EQ(optionals.Get_bytes_type().size(), helloWorldStr.length());
   EXPECT_EQ(0, memcmp(optionals.Get_bytes_type().data(), helloWorldStr.c_str(), helloWorldStr.length()));
}

TEST_F(JSON_DecodeTests, decode_tag_and_push_submessage)
{
   CheckJsonParseOK("{\"submsg_type\": { \"substuff1\": \"MyValue\" } }");

   EXPECT_TRUE(IsSet("submsg_type"));

   auto subMessage = optionals.Get_submsg_type();

   EXPECT_TRUE(IsSet(subMessage.RawData(), "substuff1"));
   EXPECT_STREQ(subMessage.Get_substuff1().c_str(), "MyValue");
}

TEST_F(JSON_DecodeTests, DecodeValue_enumFieldWithValidIntegerValue_DoesntThrow)
{
   CheckJsonParseOK("{\"enum_type\": 42}");
}

TEST_F(JSON_DecodeTests, DecodeValue_enumFieldWithValidStringValue_DoesntThrow)
{
   CheckJsonParseOK("{\"enum_type\": \"Truth\"}");
}

TEST_F(JSON_DecodeTests, DecodeValue_enumFieldWithInvalidIntegerValue_throwsBadStreamFormat)
{
   CheckJsonParseFail(
      "{\"enum_type\": -45}",
      "enum_type: '-45}' not in this enum");
}

TEST_F(JSON_DecodeTests, DecodeValue_enumFieldWithInvalidStringValue_throwsBadStreamFormat)
{
   CheckJsonParseFail(
      "{\"enum_type\": \"Invalid\"}",
      "enum_type: 'Invalid' not in this enum");
}

TEST_F(JSON_DecodeTests, DecodeValue_enumFieldWithValidIntegerValue_callsInt32PushValue)
{
   CheckJsonParseOK("{\"enum_type\": 42}");
   EXPECT_EQ(jude::TestEnum::Truth /* == 42 */, optionals.Get_enum_type());
}

TEST_F(JSON_DecodeTests, DecodeValue_enumFieldWithValidStringValue_callsStringPushValue)
{
   CheckJsonParseOK("{\"enum_type\": \"Truth\"}");
   EXPECT_EQ(jude::TestEnum::Truth, optionals.Get_enum_type());
}

TEST_F(JSON_DecodeTests, DecodeValue_bitmask_with_integer)
{
   CheckJsonParseOK("{\"bitmask_type\": 5}");

   jude::BitMask8 mask(optionals, jude::AllOptionalTypes::Index::bitmask_type);

   EXPECT_TRUE (mask.Is_BitZero());
   EXPECT_FALSE(mask.Is_BitOne());
   EXPECT_TRUE (mask.Is_BitTwo());
   EXPECT_FALSE(mask.Is_BitThree());
   EXPECT_FALSE(mask.Is_BitFour());
   EXPECT_FALSE(mask.Is_BitFive());
   EXPECT_FALSE(mask.Is_BitSix());
   EXPECT_FALSE(mask.Is_BitSeven());
}

TEST_F(JSON_DecodeTests, DecodeValue_bitmask_with_empty_string_array_does_not_fail)
{
   CheckJsonParseOK("{\"bitmask_type\": {}}");

   jude::BitMask8 mask(optionals, jude::AllOptionalTypes::Index::bitmask_type);

   EXPECT_FALSE(mask.Is_BitZero());
   EXPECT_FALSE(mask.Is_BitOne());
   EXPECT_FALSE(mask.Is_BitTwo());
   EXPECT_FALSE(mask.Is_BitThree());
   EXPECT_FALSE(mask.Is_BitFour());
   EXPECT_FALSE(mask.Is_BitFive());
   EXPECT_FALSE(mask.Is_BitSix());
   EXPECT_FALSE(mask.Is_BitSeven());
}

TEST_F(JSON_DecodeTests, DecodeValue_bitmask_with_strings_changes_specified_bits)
{
   // Given
   jude::BitMask8 mask(optionals, jude::AllOptionalTypes::Index::bitmask_type);
   mask.Set_BitOne();
   mask.Set_BitTwo();

   // When
   CheckJsonParseOK("{\"bitmask_type\": { \"BitZero\" : true, \"BitThree\":true, \"BitTwo\":false , \"BitOne\":false   } }");
   
   // Then
   EXPECT_TRUE (mask.Is_BitZero());
   EXPECT_FALSE(mask.Is_BitOne());
   EXPECT_FALSE(mask.Is_BitTwo());
   EXPECT_TRUE (mask.Is_BitThree());
   EXPECT_FALSE(mask.Is_BitFour());
   EXPECT_FALSE(mask.Is_BitFive());
   EXPECT_FALSE(mask.Is_BitSix());
   EXPECT_FALSE(mask.Is_BitSeven());
}

TEST_F(JSON_DecodeTests, DecodeValue_bitmask_with_unknown_strings_is_OK_for_backwards_compatibility)
{
   // Given
   jude::BitMask8 mask(optionals, jude::AllOptionalTypes::Index::bitmask_type);
   mask.Set_BitZero();
   mask.Set_BitThree();

   // When
   CheckJsonParseOK("{\"bitmask_type\": { \"UnknownBitName\" : true } }");

   // Then
   EXPECT_TRUE(mask.Is_BitZero());
   EXPECT_FALSE(mask.Is_BitOne());
   EXPECT_FALSE(mask.Is_BitTwo());
   EXPECT_TRUE(mask.Is_BitThree());
   EXPECT_FALSE(mask.Is_BitFour());
   EXPECT_FALSE(mask.Is_BitFive());
   EXPECT_FALSE(mask.Is_BitSix());
   EXPECT_FALSE(mask.Is_BitSeven());
}

TEST_F(JSON_DecodeTests, DecodeValue_bitmask_with_strings_leaves_unspecified_bits_alone)
{
   // Given
   jude::BitMask8 mask(optionals, jude::AllOptionalTypes::Index::bitmask_type);
   mask.Set_BitOne();
   mask.Set_BitTwo();
   // When
   CheckJsonParseOK("{\"bitmask_type\": { \"BitZero\" : true, \"BitThree\":true} }");
   // Then
   EXPECT_TRUE(mask.Is_BitZero());
   EXPECT_TRUE(mask.Is_BitOne());
   EXPECT_TRUE(mask.Is_BitTwo());
   EXPECT_TRUE(mask.Is_BitThree());
   EXPECT_FALSE(mask.Is_BitFour());
   EXPECT_FALSE(mask.Is_BitFive());
   EXPECT_FALSE(mask.Is_BitSix());
   EXPECT_FALSE(mask.Is_BitSeven());
}
