#include <gtest/gtest.h>

#include "test_base.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

class ObjectTests : public JudeTestBase
{
};

TEST_F(ObjectTests, set_rtti_single_fields)
{
   // Given
   auto object = jude::AllOptionalTypes::New();

   // Then
   ASSERT_EQ(&object.Type(), &AllOptionalTypes_rtti);
   ASSERT_EQ(&object.Get_submsg_type().Type(), &SubMessage_rtti);
}

TEST_F(ObjectTests, set_rtti_repeated_fields)
{
   // Given
   AllRepeatedTypes_t object = { };
   EXPECT_EQ(NULL, object.__rtti);

   // When
   jude_object_set_rtti((jude_object_t*)&object, &AllRepeatedTypes_rtti);

   // Then
   ASSERT_EQ(object.__rtti, &AllRepeatedTypes_rtti);
   for (int i = 0; i < 32; i++)
   {
      ASSERT_EQ(object.m_submsg_type[i].__rtti, &SubMessage_rtti);
   }
}

TEST_F(ObjectTests, get_type)
{
   EXPECT_EQ(jude_object_get_type(object(&ptrOptionals)), &AllOptionalTypes_rtti);
   EXPECT_EQ(jude_object_get_type(object(&ptrOptionals.m_submsg_type)), &SubMessage_rtti);

   EXPECT_EQ(jude_object_get_type(repeats_object), &AllRepeatedTypes_rtti);
   EXPECT_EQ(jude_object_get_type(object(&ptrRepeats.m_submsg_type[0])), &SubMessage_rtti);
   EXPECT_EQ(jude_object_get_type(object(&ptrRepeats.m_submsg_type[10])), &SubMessage_rtti);
   EXPECT_EQ(jude_object_get_type(object(&ptrRepeats.m_submsg_type[31])), &SubMessage_rtti);
}

TEST_F(ObjectTests, get_id)
{
   // Given
   ptrRepeats.m_id = 123;

   // Then
   EXPECT_EQ(jude_object_get_id(repeats_object), 123);
}

TEST_F(ObjectTests, get_child_index)
{
   EXPECT_EQ(jude_object_get_child_index(optionals_object), 0);
   EXPECT_EQ(jude_object_get_child_index(object(&ptrOptionals.m_submsg_type)), OptionalSubMessage->index);

   EXPECT_EQ(jude_object_get_child_index(repeats_object), 0);
   EXPECT_EQ(jude_object_get_child_index(object(&ptrRepeats.m_submsg_type[0])), OptionalSubMessage->index);
   EXPECT_EQ(jude_object_get_child_index(object(&ptrRepeats.m_submsg_type[10])), OptionalSubMessage->index);
   EXPECT_EQ(jude_object_get_child_index(object(&ptrRepeats.m_submsg_type[31])), OptionalSubMessage->index);
}

TEST_F(ObjectTests, is_top_level)
{
   EXPECT_TRUE(jude_object_is_top_level(object(&ptrOptionals)));
   EXPECT_FALSE(jude_object_is_top_level(object(&ptrOptionals.m_submsg_type)));

   EXPECT_TRUE(jude_object_is_top_level(repeats_object));
   EXPECT_FALSE(jude_object_is_top_level(object(&ptrRepeats.m_submsg_type[0])));
   EXPECT_FALSE(jude_object_is_top_level(object(&ptrRepeats.m_submsg_type[10])));
   EXPECT_FALSE(jude_object_is_top_level(object(&ptrRepeats.m_submsg_type[31])));
}

TEST_F(ObjectTests, get_parent_const)
{
   EXPECT_EQ(NULL, jude_object_get_parent_const(object(&ptrOptionals)));
   EXPECT_EQ(object(&ptrOptionals), jude_object_get_parent_const(object(&ptrOptionals.m_submsg_type)));

   EXPECT_EQ(NULL, jude_object_get_parent_const(repeats_object));
   EXPECT_EQ(repeats_object, jude_object_get_parent_const(object(&ptrRepeats.m_submsg_type[0])));
   EXPECT_EQ(repeats_object, jude_object_get_parent_const(object(&ptrRepeats.m_submsg_type[10])));
   EXPECT_EQ(repeats_object, jude_object_get_parent_const(object(&ptrRepeats.m_submsg_type[31])));
}

TEST_F(ObjectTests, get_parent)
{
   EXPECT_EQ(NULL, jude_object_get_parent(object(&ptrOptionals)));
   EXPECT_EQ(object(&ptrOptionals), jude_object_get_parent(object(&ptrOptionals.m_submsg_type)));

   EXPECT_EQ(NULL, jude_object_get_parent(repeats_object));
   EXPECT_EQ(repeats_object, jude_object_get_parent(object(&ptrRepeats.m_submsg_type[0])));
   EXPECT_EQ(repeats_object, jude_object_get_parent(object(&ptrRepeats.m_submsg_type[10])));
   EXPECT_EQ(repeats_object, jude_object_get_parent(object(&ptrRepeats.m_submsg_type[31])));
}

TEST_F(ObjectTests, get_mask_const)
{
   EXPECT_EQ(ptrOptionals.__mask, jude_object_get_mask_const(object(&ptrOptionals)));
}

TEST_F(ObjectTests, get_mask)
{
   EXPECT_EQ(ptrOptionals.__mask, jude_object_get_mask(object(&ptrOptionals)));
}

TEST_F(ObjectTests, find_subresource_single_subresource)
{
   EXPECT_EQ(NULL, jude_object_find_subresource(object(&ptrOptionals),  0,  0));
   EXPECT_EQ(NULL, jude_object_find_subresource(object(&ptrOptionals),  1,  0));
   EXPECT_EQ(NULL, jude_object_find_subresource(object(&ptrOptionals),  0,  1));
   EXPECT_EQ(NULL, jude_object_find_subresource(object(&ptrOptionals),  1,  1));

   // SubMessage Expected when I search in right place...
   EXPECT_EQ(object(&ptrOptionals.m_submsg_type), jude_object_find_subresource(object(&ptrOptionals), OptionalSubMessage->index,  0));
}

TEST_F(ObjectTests, find_subresource_repeated_subresource)
{
   repeats.Get_submsg_types().clear();

   EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  0,  0));
   EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  1,  0));
   EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  0,  1));
   EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  1,  1));

   // ensure that we think our array is touched
   jude_filter_set_touched(ptrRepeats.__mask, OptionalSubMessage->index, true);

   for (jude_size_t index = 0; index < 32; index ++)
   {
      jude_id_t id = (jude_size_t)(100 - index);
      // no id set yet
      EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  OptionalSubMessage->index, id));

      ptrRepeats.m_submsg_type[index].m_id = id;
      // id value assigned but id is not marked as "touched"
      EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  OptionalSubMessage->index, id));

      jude_filter_set_touched(ptrRepeats.m_submsg_type[index].__mask, 0, true);
      // id set, and marked but array not long enough
      EXPECT_EQ(NULL, jude_object_find_subresource(repeats_object,  OptionalSubMessage->index, id));

      ptrRepeats.m_submsg_type_count = (jude_size_t)(index + 1);
      // everything in place!
      EXPECT_EQ(object(&ptrRepeats.m_submsg_type[index]), jude_object_find_subresource(repeats_object, OptionalSubMessage->index,  id));
   }
}

TEST_F(ObjectTests, count_field)
{
   // Given
   jude_size_t fieldIndexInt8type = 1;

   // Singles
   jude_filter_set_touched(ptrOptionals.__mask, fieldIndexInt8type, false);
   EXPECT_EQ(0, jude_object_count_field(object(&ptrOptionals), fieldIndexInt8type));
   jude_filter_set_touched(ptrOptionals.__mask, fieldIndexInt8type, true);
   EXPECT_EQ(1, jude_object_count_field(object(&ptrOptionals), fieldIndexInt8type));

   // Repeated
   jude_filter_set_touched(ptrRepeats.__mask, fieldIndexInt8type, false);
   EXPECT_EQ(0, jude_object_count_field(repeats_object, fieldIndexInt8type));
   ptrRepeats.m_int8_type_count = 7;
   EXPECT_EQ(0, jude_object_count_field(repeats_object, fieldIndexInt8type));
   jude_filter_set_touched(ptrRepeats.__mask, fieldIndexInt8type, true);
   EXPECT_EQ(7, jude_object_count_field(repeats_object, fieldIndexInt8type));
}

TEST_F(ObjectTests, count_field_ref)
{
   // Given
   auto optionalField = jude_rtti_find_field(&AllOptionalTypes_rtti, "int8_type");
   auto repeatedField = jude_rtti_find_field(&AllRepeatedTypes_rtti, "int8_type");

   // then
   EXPECT_EQ(NULL, jude_object_count_field_ref(object(&ptrOptionals), optionalField->index));
   EXPECT_EQ(&ptrRepeats.m_int8_type_count, jude_object_count_field_ref(repeats_object, repeatedField->index));
}

TEST_F(ObjectTests, is_changed)
{
   jude_object_clear_change_markers(repeats_object);
   EXPECT_FALSE(jude_object_is_changed(repeats_object));

   for (jude_size_t field_index = 0; field_index < AllRepeatedTypes_rtti.field_count; field_index++)
   {
      jude_filter_set_touched(ptrRepeats.__mask, field_index, true);
      EXPECT_FALSE(jude_object_is_changed(repeats_object));
      jude_filter_set_changed(ptrRepeats.__mask, field_index, true);
      EXPECT_TRUE(jude_object_is_changed(repeats_object));
      jude_filter_set_touched(ptrRepeats.__mask, field_index, false);
      EXPECT_TRUE(jude_object_is_changed(repeats_object));
      jude_filter_set_changed(ptrRepeats.__mask, field_index, false);
      EXPECT_FALSE(jude_object_is_changed(repeats_object));
   }

   jude_filter_set_changed(ptrRepeats.__mask, 0, true);
   for (jude_size_t field_index = 1; field_index < AllRepeatedTypes_rtti.field_count; field_index++)
   {
      jude_filter_set_changed(ptrRepeats.__mask, field_index, true);
      EXPECT_TRUE(jude_object_is_changed(repeats_object));
      jude_filter_set_changed(ptrRepeats.__mask, field_index, false);
      EXPECT_TRUE(jude_object_is_changed(repeats_object));
   }
}

TEST_F(ObjectTests, clear_change_markers)
{
   EXPECT_FALSE(jude_object_is_changed(empty_object));

   for (jude_size_t field_index = 0; field_index < AllRepeatedTypes_rtti.field_count; field_index++)
   {
      jude_filter_set_changed(empty.RawData()->__mask, field_index, true);
      EXPECT_TRUE(jude_object_is_changed(empty_object));
      jude_object_clear_change_markers(empty_object);
      EXPECT_FALSE(jude_object_is_changed(empty_object));
   }
}

TEST_F(ObjectTests, clear_touch_markers)
{
   EXPECT_FALSE(jude_object_is_touched(empty_object));

   for (jude_size_t field_index = 0; field_index < AllRepeatedTypes_rtti.field_count; field_index++)
   {
      jude_filter_set_touched(empty.RawData()->__mask, field_index, true);
      EXPECT_TRUE(jude_object_is_touched(empty_object));
      jude_object_clear_touch_markers(empty_object);
      EXPECT_FALSE(jude_object_is_touched(empty_object));
   }
}

TEST_F(ObjectTests, clear_all)
{   
   EXPECT_FALSE(jude_object_is_changed(empty_object));
   EXPECT_FALSE(jude_object_is_touched(empty_object));

   for (jude_size_t field_index = 0; field_index < AllOptionalTypes_rtti.field_count; field_index++)
   {
      jude_filter_set_changed(empty.RawData()->__mask, field_index, true);
      EXPECT_TRUE(jude_object_is_changed(empty_object));
      jude_filter_set_touched(empty.RawData()->__mask, field_index, true);
      EXPECT_TRUE(jude_object_is_touched(empty_object));

      jude_object_clear_all(empty_object);
      EXPECT_FALSE(jude_object_is_changed(empty_object));
      EXPECT_FALSE(jude_object_is_touched(empty_object));
   }

}

TEST_F(ObjectTests, compare)
{
   auto lhs = jude::AllRepeatedTypes::New();
   Initialise_AllRepeatedTypes(lhs);
   auto rhs = jude::AllRepeatedTypes::New();
   Initialise_AllRepeatedTypes(rhs);

   // two randoms should not match
   ASSERT_NE(0, jude_object_compare(object(lhs), object(rhs)));

   // data copy
   memcpy(lhs.RawData(), rhs.RawData(), sizeof(AllRepeatedTypes_t));
   ASSERT_EQ(0, jude_object_compare(object(lhs), object(rhs)));

   // make a diff
   lhs.Get_bool_types().Add(true);
   ASSERT_NE(0, jude_object_compare(object(lhs), object(rhs)));
}

TEST_F(ObjectTests, copy_fails_if_types_not_equal_IgnoreMemLeaks)
{
   optionals.Set_bool_type(true);

   ASSERT_DEATH(jude_object_overwrite_data(optionals_object, repeats_object, true), "");
   ASSERT_DEATH(jude_object_overwrite_data(repeats_object, optionals_object, true), "");
}

TEST_F(ObjectTests, copy_optionals)
{
   // Given
   auto temp = jude::AllOptionalTypes::New();
   jude_object_t* temp_object = temp.RawData();
   Initialise_AllOptionalTypes(temp);

   // When temp has false, optionals has true...
   temp.Set_bool_type(false);
   optionals.Set_bool_type(true);

   // Then
   ASSERT_NE(0, jude_object_compare(temp_object, optionals_object));
   ASSERT_TRUE(jude_object_copy_data(temp_object, optionals_object));
   ASSERT_EQ(0, jude_object_compare(temp_object, optionals_object));
   ASSERT_EQ(true, temp.Get_bool_type());

   // And subsequent copy returns false as no change...
   ASSERT_FALSE(jude_object_copy_data(temp_object, optionals_object));
}

TEST_F(ObjectTests, copy_repeated_types)
{
   // Given
   auto temp = jude::AllRepeatedTypes::New();
   jude_object_t* temp_object = temp.RawData();
   Initialise_AllRepeatedTypes(temp);

   // When temp has false, optionals has true...
   temp.Get_bool_types().Add(false);
   repeats.Get_bool_types().Add(true);

   // Then
   ASSERT_NE(0, jude_object_compare(temp_object, repeats_object));
   ASSERT_TRUE(jude_object_copy_data(temp_object, repeats_object));
   ASSERT_EQ(0, jude_object_compare(temp_object, repeats_object));
   ASSERT_EQ(true, temp.Get_bool_type(0));

   // And subsequent copy returns false as no change...
   ASSERT_FALSE(jude_object_merge_data(temp_object, repeats_object));

   // When temp has more values than ptrOptionals...
   temp.Get_bool_types().Add(true);
   ASSERT_EQ(2, temp.Get_bool_types().count());

   // Then copying from object with fewer will reduce it back
   ASSERT_TRUE(jude_object_copy_data(temp_object, repeats_object));
   ASSERT_EQ(1, temp.Get_bool_types().count());

   // When repeats has more values than ptrOptionals...
   repeats.Get_bool_types().Add(true);
   ASSERT_EQ(2, ptrRepeats.m_bool_type_count);

   // Then copying from object with more will increase it
   ASSERT_TRUE(jude_object_copy_data(temp_object, repeats_object));
   ASSERT_EQ(2, temp.Get_bool_types().count());
}

TEST_F(ObjectTests, copy_asserts_for_nulls_and_different_types)
{
   ASSERT_DEATH(jude_object_copy_data(nullptr, nullptr), "");
   ASSERT_DEATH(jude_object_copy_data(nullptr, repeats_object), "");
   ASSERT_DEATH(jude_object_copy_data(optionals_object, nullptr), "");
   ASSERT_DEATH(jude_object_copy_data(optionals_object, repeats_object), "");
}

TEST_F(ObjectTests, DISABLED_merge)
{
   //bool result = jude_object_merge(jude_object_t *destination, const jude_object_t *source);
   FAIL() << "TODO!";
}

TEST_F(ObjectTests, SetFieldAsNumber)
{
   auto aotypes = jude::AllOptionalTypes::New();
   
   aotypes.SetFieldAsNumber<uint8_t>(jude::AllOptionalTypes::Index::uint8_type, 15);
   ASSERT_EQ(15, aotypes.Get_uint8_type_or(0));

   // Check safeety of getting this wrong
   aotypes.SetFieldAsNumber<uint16_t>(jude::AllOptionalTypes::Index::uint8_type, 15000);
   ASSERT_EQ((15000 & 0xFF), aotypes.Get_uint8_type_or(0));

   aotypes.SetFieldAsNumber<int64_t>(jude::AllOptionalTypes::Index::int64_type, -1234567890);
   ASSERT_EQ(-1234567890, aotypes.Get_int64_type_or(0));

   auto artypes = jude::AllRepeatedTypes::New();

   auto bytesArray = artypes.Get_uint8_types();
   bytesArray.Add(12);
   bytesArray.Add(45);
   bytesArray.Add(78);

   artypes.SetFieldAsNumber<uint8_t>(jude::AllOptionalTypes::Index::uint8_type, 15, 1);
   ASSERT_EQ(12, bytesArray[0]);
   ASSERT_EQ(15, bytesArray[1]);
   ASSERT_EQ(78, bytesArray[2]);

   // Check safeety of getting this wrong
   artypes.SetFieldAsNumber<uint16_t>(jude::AllOptionalTypes::Index::uint8_type, 15000, 1);
   ASSERT_EQ(12, bytesArray[0]);
   ASSERT_EQ((15000 & 0xFF), bytesArray[1]);
   ASSERT_EQ(78, bytesArray[2]);
}

