#include <gtest/gtest.h>

#include "jude/jude.h"
#include "core/test_base.h"

class IteratorTests : public JudeTestBase
{

};

TEST_F(IteratorTests, begin)
{
   // When
   auto iterator = jude_iterator_begin(repeats_object);

   // Then
   ASSERT_EQ(&AllRepeatedTypes_rtti.field_list[0], iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_id, iterator.details.data);
   ASSERT_EQ(repeats_object, iterator.object);
}

TEST_F(IteratorTests, reset)
{
   // Given
   auto iterator = jude_iterator_begin(repeats_object);
   jude_iterator_next(&iterator);
   jude_iterator_next(&iterator);

   ASSERT_EQ(&AllRepeatedTypes_rtti.field_list[2], iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_int16_type, iterator.details.data);
   ASSERT_EQ(&ptrRepeats.m_int16_type_count, jude_iterator_get_count_reference(&iterator));
   ASSERT_EQ(repeats_object, iterator.object);

   // When
   jude_iterator_reset(&iterator);

   // Then
   ASSERT_EQ(&AllRepeatedTypes_rtti.field_list[0], iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_id, iterator.details.data);
   ASSERT_EQ(repeats_object, iterator.object);
}

TEST_F(IteratorTests, next)
{
   // Given
   auto iterator = jude_iterator_begin(repeats_object);

   // When
   ASSERT_TRUE(jude_iterator_next(&iterator));

   // Then
   ASSERT_EQ(&AllRepeatedTypes_rtti.field_list[1], iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_int8_type, iterator.details.data);
   ASSERT_EQ(&ptrRepeats.m_int8_type_count, jude_iterator_get_count_reference(&iterator));
   ASSERT_EQ(repeats_object, iterator.object);

   // ... and when
   ASSERT_TRUE(jude_iterator_next(&iterator));

   // Then
   ASSERT_EQ(&AllRepeatedTypes_rtti.field_list[2], iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_int16_type, iterator.details.data);
   ASSERT_EQ(&ptrRepeats.m_int16_type_count, jude_iterator_get_count_reference(&iterator));
   ASSERT_EQ(repeats_object, iterator.object);
}

TEST_F(IteratorTests, find)
{
   // Given
   auto iterator = jude_iterator_begin(repeats_object);
   auto int32_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "int32_type");

   // When
   ASSERT_TRUE(jude_iterator_find(&iterator, int32_field->tag));

   // Then
   ASSERT_EQ(int32_field, iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_int32_type, iterator.details.data);
   ASSERT_EQ(&ptrRepeats.m_int32_type_count, jude_iterator_get_count_reference(&iterator));
   ASSERT_EQ(repeats_object, iterator.object);
}

TEST_F(IteratorTests, find_by_name)
{
   // Given
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");

   // When
   ASSERT_TRUE(jude_iterator_find_by_name(&iterator, "string_type"));

   // Then
   ASSERT_EQ(string_field, iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_string_type, iterator.details.data);
   ASSERT_EQ(&ptrRepeats.m_string_type_count, jude_iterator_get_count_reference(&iterator));
   ASSERT_EQ(repeats_object, iterator.object);
}

TEST_F(IteratorTests, go_to_index)
{
   // Given
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");

   // When
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));

   // Then
   ASSERT_EQ(string_field, iterator.current_field);
   ASSERT_EQ(&ptrRepeats.m_string_type, iterator.details.data);
   ASSERT_EQ(&ptrRepeats.m_string_type_count, jude_iterator_get_count_reference(&iterator));
   ASSERT_EQ(repeats_object, iterator.object);
}

TEST_F(IteratorTests, is_touched)
{
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));

   jude_filter_set_touched(ptrRepeats.__mask, string_field->index, false);
   jude_filter_set_changed(ptrRepeats.__mask, string_field->index, false);
   ASSERT_FALSE(jude_iterator_is_touched(&iterator));

   jude_filter_set_changed(ptrRepeats.__mask, string_field->index, true);
   ASSERT_FALSE(jude_iterator_is_touched(&iterator));

   jude_filter_set_touched(ptrRepeats.__mask, string_field->index, true);
   ASSERT_TRUE(jude_iterator_is_touched(&iterator));
}

TEST_F(IteratorTests, set_touched)
{
   jude_object_clear_all(repeats_object);
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));
   ASSERT_FALSE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_FALSE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));
   
   // When
   jude_iterator_set_touched(&iterator);

   // Then
   ASSERT_TRUE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   // And the filter is marked changed because we "touched" it 
   ASSERT_TRUE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));
   
   // But when changes are cleared and we touch it again...
   jude_iterator_clear_changed(&iterator);
   jude_iterator_set_touched(&iterator);
   // This time there are no "changes" because it was already touched
   ASSERT_FALSE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));
}

TEST_F(IteratorTests, clear_touched)
{
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));
   jude_filter_set_touched(ptrRepeats.__mask, string_field->index, true);
   jude_filter_set_changed(ptrRepeats.__mask, string_field->index, true);
   ASSERT_TRUE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_TRUE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));

   // When
   jude_iterator_clear_touched(&iterator);

   // Then
   ASSERT_FALSE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_TRUE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));
}

TEST_F(IteratorTests, is_changed)
{
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));

   jude_filter_set_touched(ptrRepeats.__mask, string_field->index, false);
   jude_filter_set_changed(ptrRepeats.__mask, string_field->index, false);
   ASSERT_FALSE(jude_iterator_is_changed(&iterator));

   jude_filter_set_touched(ptrRepeats.__mask, string_field->index, true);
   ASSERT_FALSE(jude_iterator_is_changed(&iterator));

   jude_filter_set_changed(ptrRepeats.__mask, string_field->index, true);
   ASSERT_TRUE(jude_iterator_is_changed(&iterator));
}

TEST_F(IteratorTests, set_changed)
{
   jude_object_clear_all(repeats_object);
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));
   ASSERT_FALSE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_FALSE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));

   // When
   jude_iterator_set_changed(&iterator);

   // Then
   ASSERT_FALSE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_TRUE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));
}

TEST_F(IteratorTests, clear_changed)
{
   auto iterator = jude_iterator_begin(repeats_object);
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));
   jude_filter_set_touched(ptrRepeats.__mask, string_field->index, true);
   jude_filter_set_changed(ptrRepeats.__mask, string_field->index, true);
   ASSERT_TRUE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_TRUE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));

   // When
   jude_iterator_clear_changed(&iterator);

   // Then
   ASSERT_TRUE(jude_filter_is_touched(ptrRepeats.__mask, string_field->index));
   ASSERT_FALSE(jude_filter_is_changed(ptrRepeats.__mask, string_field->index));
}

TEST_F(IteratorTests, is_array)
{
   // Repeated fields
   auto iterator = jude_iterator_begin(repeats_object);
   ASSERT_FALSE(jude_iterator_is_array(&iterator)); // id field is always first and that is not an array!
   auto string_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "string_type");
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, string_field->index));
   ASSERT_TRUE(jude_iterator_is_array(&iterator));

   // Optional fields
   auto optional_iter = jude_iterator_begin(optionals_object);
   ASSERT_FALSE(jude_iterator_is_array(&optional_iter)); // id field is always first and that is not an array!
   ASSERT_TRUE(jude_iterator_go_to_index(&optional_iter, string_field->index));
   ASSERT_FALSE(jude_iterator_is_array(&optional_iter)); // still false because this field is an optional
}

TEST_F(IteratorTests, is_subresource)
{
   auto iterator = jude_iterator_begin(repeats_object);
   ASSERT_FALSE(jude_iterator_is_subresource(&iterator));
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, RepeatedSubMessage->index));
   ASSERT_TRUE(jude_iterator_is_subresource(&iterator));
}

TEST_F(IteratorTests, get_data)
{
   auto iterator = jude_iterator_begin(repeats_object);
   ASSERT_EQ(&ptrRepeats.m_id, jude_iterator_get_data(&iterator, 0));
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, RepeatedSubMessage->index));
   ASSERT_EQ(&ptrRepeats.m_submsg_type[0], jude_iterator_get_data(&iterator, 0));
   ASSERT_EQ(&ptrRepeats.m_submsg_type[4], jude_iterator_get_data(&iterator, 4));
}

TEST_F(IteratorTests, get_size)
{
   auto iterator = jude_iterator_begin(repeats_object);
   ASSERT_EQ(sizeof(ptrRepeats.m_id), jude_iterator_get_size(&iterator));
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, RepeatedSubMessage->index));
   ASSERT_EQ(sizeof(ptrRepeats.m_submsg_type[0]), jude_iterator_get_size(&iterator));

   auto optional_iter = jude_iterator_begin(optionals_object);
   ASSERT_EQ(sizeof(ptrOptionals.m_id), jude_iterator_get_size(&optional_iter));
   ASSERT_TRUE(jude_iterator_go_to_index(&optional_iter, OptionalSubMessage->index));
   ASSERT_EQ(sizeof(ptrOptionals.m_submsg_type), jude_iterator_get_size(&optional_iter));
}

TEST_F(IteratorTests, get_count)
{
   auto iterator = jude_iterator_begin(repeats_object);
   ASSERT_EQ(0, jude_iterator_get_count(&iterator));
   jude_filter_set_touched(ptrRepeats.__mask, 0, true); // id is now set...
   ASSERT_EQ(1, jude_iterator_get_count(&iterator));

   jude_filter_set_touched(ptrRepeats.__mask, RepeatedSubMessage->index, false);
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, RepeatedSubMessage->index));
   ASSERT_EQ(0, jude_iterator_get_count(&iterator));
   
   ptrRepeats.m_submsg_type_count = 5;
   ASSERT_EQ(0, jude_iterator_get_count(&iterator));

   jude_filter_set_touched(ptrRepeats.__mask, RepeatedSubMessage->index, true);
   ASSERT_EQ(5, jude_iterator_get_count(&iterator));
}

TEST_F(IteratorTests, get_count_reference)
{
   auto iterator = jude_iterator_begin(repeats_object);
   ASSERT_EQ(NULL, jude_iterator_get_count_reference(&iterator));
   ASSERT_TRUE(jude_iterator_go_to_index(&iterator, RepeatedSubMessage->index));
   ASSERT_EQ(&ptrRepeats.m_submsg_type_count, jude_iterator_get_count_reference(&iterator));

   auto optional_iter = jude_iterator_begin(optionals_object);
   ASSERT_EQ(NULL, jude_iterator_get_count_reference(&optional_iter));
   ASSERT_TRUE(jude_iterator_go_to_index(&optional_iter, OptionalSubMessage->index));
   ASSERT_EQ(NULL, jude_iterator_get_count_reference(&optional_iter));
}

