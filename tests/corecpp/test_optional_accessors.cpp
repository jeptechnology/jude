#include <gtest/gtest.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/core/cpp/Stream.h"

class ClassOptionalAccessorsTests : public JudeTestBase
{
public:
   jude::AllOptionalTypes optionalTypes = jude::AllOptionalTypes::New();
   jude::AllRepeatedTypes repeatedTypes = jude::AllRepeatedTypes::New();

   void CheckAllFieldsUnsetExcept(jude_size_t fieldIndex, const jude::Object& object, unsigned line)
   {
      for (jude_index_t i = 0; i < object.Type().field_count; i++)
      {
         if (i == fieldIndex)
         {
            ASSERT_TRUE(object.Has(i)) << "index " << i << " in line " << line;
         }
         else
         {
            ASSERT_FALSE(object.Has(i)) << "index " << i << " in line " << line;
         }
      }
   }

   void CheckAllFieldsUnset(const jude::Object& object, unsigned line)
   {
      for (jude_index_t i = 0; i < object.Type().field_count; i++)
      {
         ASSERT_FALSE(object.Has(i)) << "index " << i << " in line " << line;
      }
   }  
};

TEST_F(ClassOptionalAccessorsTests, test_init_fields_nothing_set)
{
   CheckAllFieldsUnset(optionalTypes, __LINE__);
   ASSERT_STREQ("{}", optionalTypes.ToJSON().c_str());

   CheckAllFieldsUnset(repeatedTypes, __LINE__);
   ASSERT_STREQ("{}", repeatedTypes.ToJSON().c_str());
}

#define TEST_DEAFULT(fieldName, value1, value2)                         \
                                                                        \
   optionalTypes.Clear_ ## fieldName();                                 \
   ASSERT_FALSE(optionalTypes.Has_ ## fieldName());                     \
   ASSERT_EQ(value1, optionalTypes.Get_ ## fieldName ## _or(value1));   \
   ASSERT_EQ(value2, optionalTypes.Get_ ## fieldName ## _or(value2));   \
                                                                        \
   optionalTypes.Set_ ## fieldName(value1);                             \
   ASSERT_TRUE(optionalTypes.Has_ ## fieldName());                      \
   ASSERT_EQ(value1, optionalTypes.Get_ ## fieldName ## _or(value2));   \
   ASSERT_EQ(value1, optionalTypes.Get_ ## fieldName ## _or(value1));   \
                                                                        \
   optionalTypes.Set_ ## fieldName(value1).Set_ ## fieldName(value2);   \
   ASSERT_TRUE(optionalTypes.Has_ ## fieldName());                      \
   ASSERT_EQ(value2, optionalTypes.Get_ ## fieldName ## _or(value2));   \
   ASSERT_EQ(value2, optionalTypes.Get_ ## fieldName ## _or(value1))    \


#define CHECK_JSON_AFTER_VALUE_SET(fieldName, value) \
   optionalTypes.Set_ ## fieldName(value); \
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::fieldName, optionalTypes, __LINE__); \
   ASSERT_STREQ("{\"" #fieldName "\":" #value "}", optionalTypes.ToJSON().c_str()) \

#define CHECK_CLEAR(fieldName, defaultValue) \
   optionalTypes.Clear_ ## fieldName(); \
   ASSERT_FALSE(optionalTypes.Has_ ## fieldName()); \
   ASSERT_EQ(defaultValue, optionalTypes.Get_ ## fieldName ## _or(defaultValue)); \
   CheckAllFieldsUnset(optionalTypes, __LINE__); \

#define TEST_GENERATED_FIELD_ACCESSORS(fieldName, value1, value2) \
   TEST_DEAFULT(fieldName, value1, value2); \
   CHECK_JSON_AFTER_VALUE_SET(fieldName, value1); \
   CHECK_JSON_AFTER_VALUE_SET(fieldName, value2); \
   CHECK_CLEAR(fieldName, value1) \


TEST_F(ClassOptionalAccessorsTests, test_optional_bool_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(bool_type, true, false);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_int8_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(int8_type, -23, 45);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_int16_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(int16_type, -2223, 4555);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_int32_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(int32_type, -47483648, 7483648);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_int64_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(int64_type, -147483648, 17483648);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_uint8_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(uint8_type, 0, 45);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_uint16_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(uint16_type, 0, 4555);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_uint32_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(uint32_type, 0, 7483648);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_uint64_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(uint64_type, 0, 17483648);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_string_fields)
{
   TEST_GENERATED_FIELD_ACCESSORS(string_type, "Hello", "World");
}

TEST_F(ClassOptionalAccessorsTests, test_optional_string_truncation)
{
   optionalTypes.Set_string_type("1234567890123456789012345678901234567890");
   // only 31 chars are expected as size of "string_type" in our schema is max 32
   ASSERT_STREQ("1234567890123456789012345678901", optionalTypes.Get_string_type().c_str());
}

TEST_F(ClassOptionalAccessorsTests, test_optional_bytes_fields)
{
   uint8_t data[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33 };
   
   optionalTypes.Clear_bytes_type();
   ASSERT_FALSE(optionalTypes.Has_bytes_type());
   ASSERT_EQ(0, optionalTypes.Get_bytes_type().size());
   
   optionalTypes.Set_bytes_type(data, 5);
   ASSERT_TRUE(optionalTypes.Has_bytes_type());
   ASSERT_EQ(5, optionalTypes.Get_bytes_type().size());
   for (int i = 0; i < 5; i++) 
   {
      ASSERT_EQ(i, optionalTypes.Get_bytes_type()[i]);
   }

   optionalTypes.Set_bytes_type(data, 33);
   ASSERT_TRUE(optionalTypes.Has_bytes_type());
   ASSERT_EQ(32, optionalTypes.Get_bytes_type().size()); // truncated
   for (int i = 0; i < 32; i++)
   {
      ASSERT_EQ(i, optionalTypes.Get_bytes_type()[i]);
   }

   std::vector<uint8_t> vec{ 10,9,8,7,6,5,4,3,2,1 };
   optionalTypes.Set_bytes_type(vec);
   ASSERT_TRUE(optionalTypes.Has_bytes_type());
   ASSERT_EQ(10, optionalTypes.Get_bytes_type().size());
   for (int i = 0; i < 10; i++)
   {
      ASSERT_EQ(10 - i, optionalTypes.Get_bytes_type()[i]);
   }

   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bytes_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"bytes_type":"CgkIBwYFBAMCAQ=="})", optionalTypes.ToJSON().c_str());

   optionalTypes.Clear_bytes_type();
   ASSERT_FALSE(optionalTypes.Has_bytes_type());
   ASSERT_EQ(0, optionalTypes.Get_bytes_type().size());
}

TEST_F(ClassOptionalAccessorsTests, test_optional_submsg_fields)
{
   optionalTypes.Clear_submsg_type();
   ASSERT_FALSE(optionalTypes.Has_submsg_type());
   ASSERT_STREQ("{}", optionalTypes.ToJSON().c_str());
   
   auto submsg = optionalTypes.Get_submsg_type();
   ASSERT_FALSE(optionalTypes.Has_submsg_type()) << "Just getting does not touch sub object";
   ASSERT_STREQ("{}", optionalTypes.ToJSON().c_str());

   submsg.Set_substuff1("Hello");
   ASSERT_TRUE(optionalTypes.Has_submsg_type());
   ASSERT_STREQ(R"({"submsg_type":{"substuff1":"Hello"}})", optionalTypes.ToJSON().c_str());

   submsg.Clear();
   ASSERT_STREQ(R"({"submsg_type":{"substuff1":null}})", optionalTypes.ToJSON().c_str());
   submsg.ClearChangeMarkers();
   ASSERT_STREQ(R"({"submsg_type":{}})", optionalTypes.ToJSON().c_str());

   optionalTypes.Clear_submsg_type();
   ASSERT_FALSE(optionalTypes.Has_submsg_type());
   ASSERT_STREQ(R"({"submsg_type":null})", optionalTypes.ToJSON().c_str());
   optionalTypes.ClearChangeMarkers();
   ASSERT_STREQ("{}", optionalTypes.ToJSON().c_str());
}

TEST_F(ClassOptionalAccessorsTests, test_optional_enum_fields)
{
   TEST_DEAFULT(enum_type, jude::TestEnum::First, jude::TestEnum::Second);

   optionalTypes.Set_enum_type(jude::TestEnum::Truth);
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::enum_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"enum_type":"Truth"})", optionalTypes.ToJSON().c_str());

   optionalTypes.Set_enum_type(jude::TestEnum::Zero);
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::enum_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"enum_type":"Zero"})", optionalTypes.ToJSON().c_str());

   CHECK_CLEAR(enum_type, jude::TestEnum::Second);
}

TEST_F(ClassOptionalAccessorsTests, test_optional_bit_fields)
{
   CheckAllFieldsUnset(optionalTypes, __LINE__);
   
   auto bitmask = optionalTypes.Get_bitmask_type();
   ASSERT_FALSE(optionalTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnset(optionalTypes, __LINE__);
   ASSERT_STREQ(R"({})", optionalTypes.ToJSON().c_str());

   bitmask.Set_BitFive();
   ASSERT_TRUE(optionalTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":["BitFive"]})", optionalTypes.ToJSON().c_str());

   bitmask.Set_BitTwo();
   ASSERT_TRUE(optionalTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":["BitTwo","BitFive"]})", optionalTypes.ToJSON().c_str());

   bitmask.Clear_BitFive();
   ASSERT_TRUE(optionalTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":["BitTwo"]})", optionalTypes.ToJSON().c_str());

   bitmask.Clear_BitTwo();
   ASSERT_TRUE(optionalTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, optionalTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":[]})", optionalTypes.ToJSON().c_str());

   bitmask.ClearAll();
   ASSERT_FALSE(optionalTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnset(optionalTypes, __LINE__);   
   ASSERT_STREQ(R"({"bitmask_type":null})", optionalTypes.ToJSON().c_str());
   optionalTypes.ClearChangeMarkers();
   ASSERT_STREQ(R"({})", optionalTypes.ToJSON().c_str());
}

TEST_F(ClassOptionalAccessorsTests, test_unknown_field_parsing)
{
   std::string unknownField;
   std::string unknownData;

   optionalTypes.UpdateFromJson(R"({"UnknownField":"Some \" Data \" "})", [&] (const char* fieldName, const char *data) -> bool {
      unknownField = fieldName;
      unknownData = data;
      return true;
   });

   ASSERT_EQ(unknownField, "UnknownField");
   ASSERT_EQ(unknownData, R"(Some " Data " )");
}

TEST_F(ClassOptionalAccessorsTests, test_extra_field_output)
{
   std::string extraFieldName = "EXTRA";
   std::string extraData = R"(Some more "Data")";

   optionalTypes.Set_bool_type(true);
   optionalTypes.Set_int16_type(123);

   ASSERT_EQ(R"({"int16_type":123,"bool_type":true})", optionalTypes.ToJSON());

   auto json = optionalTypes.ToJSON_WithExtraField([&] (const char **extraField, const char **fieldData) -> bool {
      *extraField = extraFieldName.c_str();
      *fieldData = extraData.c_str();
      return true;
   }, jude_user_Root);

   ASSERT_EQ("{\"int16_type\":123,\"bool_type\":true,\"EXTRA\":\"Some more \\\"Data\\\"\"}", json);
}