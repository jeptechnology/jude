#include <gtest/gtest.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

class ClassArrayAccessorsTests : public JudeTestBase
{
public:
   jude::AllRepeatedTypes arrayTypes = jude::AllRepeatedTypes::New();

   void CheckAllFieldsUnsetExcept(jude_size_t fieldIndex, const jude::Object& resource, unsigned line)
   {
      for (jude_index_t i = 0; i < resource.Type().field_count; i++)
      {
         if (i == fieldIndex)
         {
            ASSERT_TRUE(resource.Has(i)) << "index " << i << " in line " << line;
         }
         else
         {
            ASSERT_FALSE(resource.Has(i)) << "index " << i << " in line " << line;
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

TEST_F(ClassArrayAccessorsTests, test_init_fields_nothing_set)
{
   CheckAllFieldsUnset(arrayTypes, __LINE__);
   ASSERT_STREQ("{}", arrayTypes.ToJSON().c_str());
}

#define CHECK_ARRAY_FIELD(text, field, type, expected, ...) do {      \
   type values[] = { __VA_ARGS__ };                                  \
   jude_size_t elementCount = (jude_size_t)(sizeof(values) / sizeof(values[0])); \
   auto testArray = arrayTypes.Get_ ## field ## s();                 \
   ASSERT_EQ(elementCount, testArray.count());                       \
   for (jude_index_t i = 0; i < elementCount; i++)                       \
   {                                                                 \
      ASSERT_EQ(values[i], testArray[i]) << text << " where i=" << i; \
   }                                                                 \
   type readdata[] = { __VA_ARGS__ };                                \
   memset(readdata, 0, sizeof(readdata));                            \
   ASSERT_TRUE(testarray.Read(readdata, elementCount));              \
   for (jude_index_t i = 0; i < elementCount; i++)                       \
   {                                                                 \
      ASSERT_EQ(values[i], readdata[i]) << text << " where i=" << i;  \
   }                                                                 \
   ASSERT_STREQ("{\"" #field "\":" expected "}", arrayTypes.ToJSON().c_str()) << text; \
} while(false)

#define TEST_GENERATED_ARRAY_FIELD_ACCESSORS(fieldName, type, value1, value2, value3) do {   \
   auto testarray = arrayTypes.Get_ ## fieldName ## s();                               \
   CheckAllFieldsUnset(arrayTypes, __LINE__);                                          \
   ASSERT_STREQ("{}", arrayTypes.ToJSON().c_str());                                    \
   ASSERT_EQ(0, testarray.count());                                                    \
                                                                                       \
   ASSERT_TRUE(testarray.Add(value1));                                                 \
   CHECK_ARRAY_FIELD("1st Add", fieldName, type, "[" #value1 "]", value1);             \
   ASSERT_TRUE(testarray.Add(value2));                                                 \
   CHECK_ARRAY_FIELD("2nd Add", fieldName, type, "[" #value1 "," #value2 "]", value1, value2);    \
   ASSERT_TRUE(testarray.Insert(1, value3));                                           \
   CHECK_ARRAY_FIELD("Insert", fieldName, type, "[" #value1 "," #value3 "," #value2 "]", value1, value3, value2);    \
                                                                                       \
   ASSERT_TRUE(testarray.Set(1, value2));                                              \
   ASSERT_TRUE(testarray.Set(2, value3));                                              \
   ASSERT_FALSE(testarray.Set(4, value3)); /* out of bounds */                         \
   CHECK_ARRAY_FIELD("3 Sets", fieldName, type, "[" #value1 "," #value2 "," #value3 "]", value1, value2, value3);    \
                                                                                       \
   ASSERT_TRUE(testarray.RemoveAt(1));   /* remove second element */                     \
   ASSERT_FALSE(testarray.RemoveAt(3));  /* can't remove 3rd element (does not exist!) */\
   CHECK_ARRAY_FIELD("Remove", fieldName, type, "[" #value1 "," #value3 "]", value1, value3);    \
                                                                                       \
   testarray.clear();                                                                  \
   ASSERT_STREQ("{\"" #fieldName "\":null}", arrayTypes.ToJSON().c_str());             \
   arrayTypes.ClearChangeMarkers();                                                    \
   ASSERT_STREQ("{}", arrayTypes.ToJSON().c_str());                                    \
   ASSERT_EQ(0, testarray.count());                                                    \
                                                                                       \
   type writeData[] = { value1, value2, value3 };                                      \
   testarray.Write(writeData, 3);                                                      \
   CHECK_ARRAY_FIELD("Write", fieldName, type, "[" #value1 "," #value2 "," #value3 "]", value1, value2, value3); \
} while(0)

TEST_F(ClassArrayAccessorsTests, test_repeated_bool_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(bool_type, bool, true, false, true);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_int8_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(int8_type, int8_t,  -23, 45, 12);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_int16_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(int16_type, int16_t, -2223, 4555, -46);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_int32_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(int32_type, int32_t, -47483648, 7483648, 345678);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_int64_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(int64_type, int64_t, -147483648, 17483648, 222);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_uint8_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(uint8_type, uint8_t, 124, 2, 45);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_uint16_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(uint16_type, uint16_t, 24, 12, 4555);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_uint32_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(uint32_type, uint32_t, 1222, 2, 7483648);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_uint64_fields)
{
   TEST_GENERATED_ARRAY_FIELD_ACCESSORS(uint64_type, uint64_t, 122224, 2, 17483648);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_string_fields)
{
   //TEST_GENERATED_ARRAY_FIELD_ACCESSORS(string_type, "Hello", "World");
}

/*
TEST_F(ClassArrayAccessorsTests, test_repeated_string_truncation)
{
   arrayTypes.Set_string_type("1234567890123456789012345678901234567890");
   // only 31 chars are expected as size of "string_type" in our schema is max 32
   ASSERT_STREQ("1234567890123456789012345678901", arrayTypes.Get_string_type().c_str());
}

TEST_F(ClassArrayAccessorsTests, test_repeated_bytes_fields)
{
   uint8_t data[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33 };
   
   arrayTypes.Clear_bytes_type();
   ASSERT_FALSE(arrayTypes.Has_bytes_type());
   ASSERT_EQ(0, arrayTypes.Get_bytes_type().size());
   
   arrayTypes.Set_bytes_type(data, 5);
   ASSERT_TRUE(arrayTypes.Has_bytes_type());
   ASSERT_EQ(5, arrayTypes.Get_bytes_type().size());
   for (int i = 0; i < 5; i++) 
   {
      ASSERT_EQ(i, arrayTypes.Get_bytes_type()[i]);
   }

   arrayTypes.Set_bytes_type(data, 33);
   ASSERT_TRUE(arrayTypes.Has_bytes_type());
   ASSERT_EQ(32, arrayTypes.Get_bytes_type().size()); // truncated
   for (int i = 0; i < 32; i++)
   {
      ASSERT_EQ(i, arrayTypes.Get_bytes_type()[i]);
   }

   std::vector<uint8_t> vec{ 10,9,8,7,6,5,4,3,2,1 };
   arrayTypes.Set_bytes_type(vec);
   ASSERT_TRUE(arrayTypes.Has_bytes_type());
   ASSERT_EQ(10, arrayTypes.Get_bytes_type().size());
   for (int i = 0; i < 10; i++)
   {
      ASSERT_EQ(10 - i, arrayTypes.Get_bytes_type()[i]);
   }

   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bytes_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"bytes_type":"CgkIBwYFBAMCAQ=="})", arrayTypes.ToJSON().c_str());

   arrayTypes.Clear_bytes_type();
   ASSERT_FALSE(arrayTypes.Has_bytes_type());
   ASSERT_EQ(0, arrayTypes.Get_bytes_type().size());
}

TEST_F(ClassArrayAccessorsTests, test_repeated_submsg_fields)
{
   arrayTypes.Clear_submsg_type();
   ASSERT_FALSE(arrayTypes.Has_submsg_type());
   ASSERT_STREQ("{}", arrayTypes.ToJSON().c_str());
   
   auto submsg = arrayTypes.Get_submsg_type();
   ASSERT_TRUE(arrayTypes.Has_submsg_type());
   ASSERT_STREQ(R"({"submsg_type":{}})", arrayTypes.ToJSON().c_str());

   submsg.Set_substuff1("Hello");
   ASSERT_TRUE(arrayTypes.Has_submsg_type());
   ASSERT_STREQ(R"({"submsg_type":{"substuff1":"Hello"}})", arrayTypes.ToJSON().c_str());

   submsg.ClearAllFields();
   ASSERT_STREQ(R"({"submsg_type":{}})", arrayTypes.ToJSON().c_str());

   arrayTypes.Clear_submsg_type();
   ASSERT_FALSE(arrayTypes.Has_submsg_type());
   ASSERT_STREQ("{}", arrayTypes.ToJSON().c_str());
}

TEST_F(ClassArrayAccessorsTests, test_repeated_enum_fields)
{
   TEST_DEAFULT(enum_type, TestEnum_First, TestEnum_Second);

   arrayTypes.Set_enum_type(TestEnum_Truth);
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::enum_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"enum_type":"Truth"})", arrayTypes.ToJSON().c_str());

   arrayTypes.Set_enum_type(TestEnum_Zero);
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::enum_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"enum_type":"Zero"})", arrayTypes.ToJSON().c_str());

   CHECK_CLEAR(enum_type, TestEnum_Second);
}

TEST_F(ClassArrayAccessorsTests, test_repeated_bit_fields)
{
   CheckAllFieldsUnset(arrayTypes, __LINE__);
   
   auto bitmask = arrayTypes.Get_bitmask_type();
   ASSERT_FALSE(arrayTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnset(arrayTypes, __LINE__);
   ASSERT_STREQ(R"({})", arrayTypes.ToJSON().c_str());

   bitmask.Set_BitFive();
   ASSERT_TRUE(arrayTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":["BitFive"]})", arrayTypes.ToJSON().c_str());

   bitmask.Set_BitTwo();
   ASSERT_TRUE(arrayTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":["BitTwo","BitFive"]})", arrayTypes.ToJSON().c_str());

   bitmask.Clear_BitFive();
   ASSERT_TRUE(arrayTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":["BitTwo"]})", arrayTypes.ToJSON().c_str());

   bitmask.Clear_BitTwo();
   ASSERT_TRUE(arrayTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnsetExcept(jude::AllOptionalTypes::Index::bitmask_type, arrayTypes, __LINE__);
   ASSERT_STREQ(R"({"bitmask_type":[]})", arrayTypes.ToJSON().c_str());

   bitmask.ClearAll();
   ASSERT_FALSE(arrayTypes.Get_bitmask_type().IsSet());
   CheckAllFieldsUnset(arrayTypes, __LINE__);
   ASSERT_STREQ(R"({})", arrayTypes.ToJSON().c_str());
}

*/