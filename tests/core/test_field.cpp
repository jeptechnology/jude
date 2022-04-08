#include <gtest/gtest.h>

#include "jude/jude.h"
#include "autogen/alltypes_test.model.h"
#include "core/test_base.h"

class FieldTests : public JudeTestBase
{
public:
   void AssertOptionalField(const char *field_name, bool (*callback)(const jude_field_t*), bool expectation)
   {
      auto field = jude_rtti_find_field(&AllOptionalTypes_rtti, field_name);
      ASSERT_EQ(expectation, callback(field)) << field_name;
   }

   void AssertRepeatedField(const char *field_name, bool (*callback)(const jude_field_t*), bool expectation)
   {
      auto field = jude_rtti_find_field(&AllRepeatedTypes_rtti, field_name);
      ASSERT_EQ(expectation, callback(field)) << field_name;
   }

   void AssertTagsTestField(const char *field_name, bool(*callback)(const jude_field_t*), bool expectation)
   {
      auto field = jude_rtti_find_field(&TagsTest_rtti, field_name);
      ASSERT_EQ(expectation, callback(field)) << field_name;
   }

   void AssertOptionalSize(const char *field_name, jude_size_t expectation)
   {
      auto field = jude_rtti_find_field(&AllOptionalTypes_rtti, field_name);
      ASSERT_EQ(expectation, jude_field_get_size(field)) << field_name;
   }

   void AssertRepeatedSize(const char *field_name, jude_size_t expectation)
   {
      auto field = jude_rtti_find_field(&AllOptionalTypes_rtti, field_name);
      ASSERT_EQ(expectation, jude_field_get_size(field)) << field_name;
   }

   void AssertArrayCount(const char *field_name, jude_size_t*countref, void *data)
   {
      auto field = jude_rtti_find_field(&AllRepeatedTypes_rtti, field_name);
      *countref = 5;
      ASSERT_EQ(5, jude_get_array_count(field, data)) << field_name;

      *countref = 10;
      ASSERT_EQ(10, jude_get_array_count(field, data)) << field_name;
   }
   
   void AssertCountReference(const char *field_name, jude_size_t*expectation, void *data)
   {
      auto field = jude_rtti_find_field(&AllRepeatedTypes_rtti, field_name);
      ASSERT_EQ(expectation, jude_get_array_count_reference(field, data)) << field_name;
   }

   void AssertArrayData(const char *field_name, jude_size_t index, void *start, void *expected)
   {
      auto field = jude_rtti_find_field(&AllRepeatedTypes_rtti, field_name);
      ASSERT_EQ(expected, jude_get_array_data(field, start, index)) << field_name;
   }

   
};

TEST_F(FieldTests, is_array)
{
   for (unsigned field_index = 0; field_index < AllOptionalTypes_rtti.field_count; field_index++)
   {
      ASSERT_FALSE(jude_field_is_array(&AllOptionalTypes_rtti.field_list[field_index]));
   }

   // Note: start from 1 due to m_id field at index 0...
   for (unsigned field_index = 1; field_index < AllRepeatedTypes_rtti.field_count; field_index++)
   {
      ASSERT_FALSE(jude_field_is_array(&AllOptionalTypes_rtti.field_list[field_index]));
   }
}

TEST_F(FieldTests, is_string)
{
   auto string_field_index = jude_rtti_find_field(&AllOptionalTypes_rtti, "string_type")->index;
   for (unsigned field_index = 0; field_index < AllOptionalTypes_rtti.field_count; field_index++)
   {
      if (field_index == string_field_index)
      {
         ASSERT_TRUE(jude_field_is_string(&AllOptionalTypes_rtti.field_list[field_index]));
      }
      else
      {
         ASSERT_FALSE(jude_field_is_string(&AllOptionalTypes_rtti.field_list[field_index]));
      }
   }

   string_field_index = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type")->index;
   for (unsigned field_index = 0; field_index < AllRepeatedTypes_rtti.field_count; field_index++)
   {
      if (field_index == string_field_index)
      {
         ASSERT_TRUE(jude_field_is_string(&AllRepeatedTypes_rtti.field_list[field_index]));
      }
      else
      {
         ASSERT_FALSE(jude_field_is_string(&AllRepeatedTypes_rtti.field_list[field_index]));
      }
   }
}

TEST_F(FieldTests, is_numeric)
{
   AssertOptionalField("int8_type",     jude_field_is_numeric, true);
   AssertOptionalField("int16_type",    jude_field_is_numeric, true);
   AssertOptionalField("uint8_type",    jude_field_is_numeric, true);
   AssertOptionalField("uint16_type",   jude_field_is_numeric, true);
   AssertOptionalField("int32_type",    jude_field_is_numeric, true);
   AssertOptionalField("int64_type",    jude_field_is_numeric, true);
   AssertOptionalField("uint32_type",   jude_field_is_numeric, true);
   AssertOptionalField("uint64_type",   jude_field_is_numeric, true);
   AssertOptionalField("bool_type",     jude_field_is_numeric, true);
   AssertOptionalField("string_type",   jude_field_is_numeric, false);
   AssertOptionalField("bytes_type",    jude_field_is_numeric, false);
   AssertOptionalField("submsg_type",   jude_field_is_numeric, false);
   AssertOptionalField("enum_type",     jude_field_is_numeric, true);
   AssertOptionalField("bitmask_type",  jude_field_is_numeric, true);
}

TEST_F(FieldTests, is_submessage)
{
   AssertOptionalField("int8_type",     jude_field_is_object, false);
   AssertOptionalField("int16_type",    jude_field_is_object, false);
   AssertOptionalField("uint8_type",    jude_field_is_object, false);
   AssertOptionalField("uint16_type",   jude_field_is_object, false);
   AssertOptionalField("int32_type",    jude_field_is_object, false);
   AssertOptionalField("int64_type",    jude_field_is_object, false);
   AssertOptionalField("uint32_type",   jude_field_is_object, false);
   AssertOptionalField("uint64_type",   jude_field_is_object, false);
   AssertOptionalField("bool_type",     jude_field_is_object, false);
   AssertOptionalField("string_type",   jude_field_is_object, false);
   AssertOptionalField("bytes_type",    jude_field_is_object, false);
   AssertOptionalField("submsg_type",   jude_field_is_object, true);
   AssertOptionalField("enum_type",     jude_field_is_object, false);
   AssertOptionalField("bitmask_type",  jude_field_is_object, false);
}

TEST_F(FieldTests, is_public_readable)
{
   AssertTagsTestField("privateStatus",        jude_field_is_public_readable, false);
   AssertTagsTestField("privateConfig",        jude_field_is_public_readable, false);
   AssertTagsTestField("action",               jude_field_is_public_readable, false);
   AssertTagsTestField("somePassword",         jude_field_is_public_readable, false);
   AssertTagsTestField("publicStatus",         jude_field_is_public_readable, true);
   AssertTagsTestField("publicReadOnlyConfig", jude_field_is_public_readable, true);
   AssertTagsTestField("publicTempConfig",     jude_field_is_public_readable, true);
   AssertTagsTestField("publicConfig",         jude_field_is_public_readable, true);
}

TEST_F(FieldTests, is_public_writable)
{
   AssertTagsTestField("privateStatus",        jude_field_is_public_writable, false);
   AssertTagsTestField("privateConfig",        jude_field_is_public_writable, false);
   AssertTagsTestField("action",               jude_field_is_public_writable, true);
   AssertTagsTestField("somePassword",         jude_field_is_public_writable, true);
   AssertTagsTestField("publicStatus",         jude_field_is_public_writable, false);
   AssertTagsTestField("publicReadOnlyConfig", jude_field_is_public_writable, false);
   AssertTagsTestField("publicTempConfig",     jude_field_is_public_writable, true);
   AssertTagsTestField("publicConfig",         jude_field_is_public_writable, true);
}

TEST_F(FieldTests, is_persisted)
{
   AssertTagsTestField("privateStatus",        jude_field_is_persisted, false);
   AssertTagsTestField("privateConfig",        jude_field_is_persisted, true);
   AssertTagsTestField("action",               jude_field_is_persisted, false);
   AssertTagsTestField("somePassword",         jude_field_is_persisted, true);
   AssertTagsTestField("publicStatus",         jude_field_is_persisted, false);
   AssertTagsTestField("publicReadOnlyConfig", jude_field_is_persisted, true);
   AssertTagsTestField("publicTempConfig",     jude_field_is_persisted, false);
   AssertTagsTestField("publicConfig",         jude_field_is_persisted, true);
}

TEST_F(FieldTests, get_size)
{
   AssertOptionalSize("int8_type",     1);
   AssertOptionalSize("int16_type",    2);
   AssertOptionalSize("int32_type",    4);
   AssertOptionalSize("int64_type",    8);

   AssertOptionalSize("uint8_type",    1);
   AssertOptionalSize("uint16_type",   2);
   AssertOptionalSize("uint32_type",   4);
   AssertOptionalSize("uint64_type",   8);

   AssertOptionalSize("bool_type",     1);
   AssertOptionalSize("string_type",   sizeof((AllOptionalTypes_t*)0)->m_string_type);
   AssertOptionalSize("bytes_type",    sizeof((AllOptionalTypes_t*)0)->m_bytes_type);
   AssertOptionalSize("submsg_type",   sizeof((AllOptionalTypes_t*)0)->m_submsg_type);
   AssertOptionalSize("enum_type",     4);
   AssertOptionalSize("bitmask_type",  1);
}

TEST_F(FieldTests, get_array_count)
{
   AssertArrayCount("int8_type",  &ptrRepeats.m_int8_type_count, ptrRepeats.m_int8_type);
   AssertArrayCount("int16_type", &ptrRepeats.m_int16_type_count, ptrRepeats.m_int16_type);
   AssertArrayCount("int32_type", &ptrRepeats.m_int32_type_count, ptrRepeats.m_int32_type);
   AssertArrayCount("int64_type", &ptrRepeats.m_int64_type_count, ptrRepeats.m_int64_type);

   AssertArrayCount("uint8_type", &ptrRepeats.m_uint8_type_count, &ptrRepeats.m_uint8_type);
   AssertArrayCount("uint16_type", &ptrRepeats.m_uint16_type_count, &ptrRepeats.m_uint16_type);
   AssertArrayCount("uint32_type", &ptrRepeats.m_uint32_type_count, &ptrRepeats.m_uint32_type);
   AssertArrayCount("uint64_type", &ptrRepeats.m_uint64_type_count, &ptrRepeats.m_uint64_type);

   AssertArrayCount("bool_type", &ptrRepeats.m_bool_type_count, &ptrRepeats.m_bool_type);
   AssertArrayCount("string_type", &ptrRepeats.m_string_type_count, &ptrRepeats.m_string_type);
   AssertArrayCount("bytes_type", &ptrRepeats.m_bytes_type_count, &ptrRepeats.m_bytes_type);
   AssertArrayCount("submsg_type", &ptrRepeats.m_submsg_type_count, &ptrRepeats.m_submsg_type);
   AssertArrayCount("enum_type", &ptrRepeats.m_enum_type_count, &ptrRepeats.m_enum_type);
   AssertArrayCount("bitmask_type", &ptrRepeats.m_bitmask_type_count, &ptrRepeats.m_bitmask_type);
}

TEST_F(FieldTests, get_array_count_reference)
{
   AssertCountReference("int8_type", &ptrRepeats.m_int8_type_count, ptrRepeats.m_int8_type);
   AssertCountReference("int16_type", &ptrRepeats.m_int16_type_count, ptrRepeats.m_int16_type);
   AssertCountReference("int32_type", &ptrRepeats.m_int32_type_count, ptrRepeats.m_int32_type);
   AssertCountReference("int64_type", &ptrRepeats.m_int64_type_count, ptrRepeats.m_int64_type);

   AssertCountReference("uint8_type", &ptrRepeats.m_uint8_type_count, ptrRepeats.m_uint8_type);
   AssertCountReference("uint16_type", &ptrRepeats.m_uint16_type_count, ptrRepeats.m_uint16_type);
   AssertCountReference("uint32_type", &ptrRepeats.m_uint32_type_count, ptrRepeats.m_uint32_type);
   AssertCountReference("uint64_type", &ptrRepeats.m_uint64_type_count, ptrRepeats.m_uint64_type);

   AssertCountReference("bool_type", &ptrRepeats.m_bool_type_count, ptrRepeats.m_bool_type);
   AssertCountReference("string_type", &ptrRepeats.m_string_type_count, ptrRepeats.m_string_type);
   AssertCountReference("bytes_type", &ptrRepeats.m_bytes_type_count, ptrRepeats.m_bytes_type);
   AssertCountReference("submsg_type", &ptrRepeats.m_submsg_type_count, ptrRepeats.m_submsg_type);
   AssertCountReference("enum_type", &ptrRepeats.m_enum_type_count, ptrRepeats.m_enum_type);
   AssertCountReference("bitmask_type", &ptrRepeats.m_bitmask_type_count, ptrRepeats.m_bitmask_type);
}

TEST_F(FieldTests, get_array_data)
{
   AssertArrayData("int8_type", 1, ptrRepeats.m_int8_type, &ptrRepeats.m_int8_type[1]);
   AssertArrayData("int16_type", 2, ptrRepeats.m_int16_type, &ptrRepeats.m_int16_type[2]);
   AssertArrayData("int32_type", 3, ptrRepeats.m_int32_type, &ptrRepeats.m_int32_type[3]);
   AssertArrayData("int64_type", 4, ptrRepeats.m_int64_type, &ptrRepeats.m_int64_type[4]);

   AssertArrayData("uint8_type", 5, ptrRepeats.m_uint8_type, &ptrRepeats.m_uint8_type[5]);
   AssertArrayData("uint16_type", 6, ptrRepeats.m_uint16_type, &ptrRepeats.m_uint16_type[6]);
   AssertArrayData("uint32_type", 7, ptrRepeats.m_uint32_type, &ptrRepeats.m_uint32_type[7]);
   AssertArrayData("uint64_type", 8, ptrRepeats.m_uint64_type, &ptrRepeats.m_uint64_type[8]);

   AssertArrayData("bool_type", 9, ptrRepeats.m_bool_type, &ptrRepeats.m_bool_type[9]);
   AssertArrayData("string_type", 10, ptrRepeats.m_string_type, &ptrRepeats.m_string_type[10]);
   AssertArrayData("bytes_type", 11, ptrRepeats.m_bytes_type, &ptrRepeats.m_bytes_type[11]);
   AssertArrayData("submsg_type", 12, ptrRepeats.m_submsg_type, &ptrRepeats.m_submsg_type[12]);
   AssertArrayData("enum_type", 13, ptrRepeats.m_enum_type, &ptrRepeats.m_enum_type[13]);
   AssertArrayData("bitmask_type", 14, ptrRepeats.m_bitmask_type, &ptrRepeats.m_bitmask_type[14]);
}
