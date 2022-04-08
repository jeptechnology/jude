#include <gtest/gtest.h>

#include "jude/jude.h"

TEST(jude_filters, empty_filter)
{
   // Given
   jude_filter_t filter = JUDE_EMPTY_FILTER;

   // Then
   EXPECT_TRUE(jude_filter_is_empty(&filter));

   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      EXPECT_FALSE(jude_filter_is_changed(filter.mask, i));
      EXPECT_FALSE(jude_filter_is_touched(filter.mask, i));
   }
}

void test_changed_bit(jude_size_t changed_bit)
{
   // Given
   jude_filter_t filter = JUDE_EMPTY_FILTER;

   // When
   jude_filter_set_changed(filter.mask, changed_bit, true);

   // Then
   EXPECT_TRUE(jude_filter_is_changed(filter.mask, changed_bit));

   // check all other bits are not changed or touched
   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      if (i == changed_bit) continue;
      EXPECT_FALSE(jude_filter_is_changed(filter.mask, i));
   }

   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      EXPECT_FALSE(jude_filter_is_touched(filter.mask, i));
   }
}

TEST(jude_filters, set_bits)
{
   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      test_changed_bit(i);
   }
}

void test_touched_bit(jude_size_t touched_bit)
{
   // Given
   jude_filter_t filter = JUDE_EMPTY_FILTER;

   // When
   jude_filter_set_touched(filter.mask, touched_bit, true);

   // Then
   EXPECT_TRUE(jude_filter_is_touched(filter.mask, touched_bit));

   // check all other bits are not changed or touched
   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      EXPECT_FALSE(jude_filter_is_changed(filter.mask, i));
   }

   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      if (i == touched_bit) continue;
      EXPECT_FALSE(jude_filter_is_touched(filter.mask, i));
   }
}

TEST(jude_filters, touched_bits)
{
   for (jude_size_t i = 0; i < JUDE_MAX_FIELDS_PER_MESSAGE; i++)
   {
      test_touched_bit(i);
   }
}

TEST(jude_filters, clear_all)
{
   // Given a filter with some random bits set
   jude_filter_t filter = JUDE_EMPTY_FILTER;
   filter.words[0] = 0xABCDEF98;
   filter.words[1] = 0x76543210;
   filter.words[2] = 0xABCDEF98;
   filter.words[3] = 0x76543210;
   EXPECT_FALSE(jude_filter_is_empty(&filter));

   // When
   jude_filter_clear_all(&filter);

   // Then
   EXPECT_TRUE(jude_filter_is_empty(&filter));
}

TEST(jude_filters, clear_all_changed)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, clear_all_touched)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, fill_all)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, fill_all_changed)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, fill_all_touched)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, is_empty)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, is_overlapping)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, and_equals)
{
   //FAIL() << "Not implemented";
}

TEST(jude_filters, or_equals)
{
   //FAIL() << "Not implemented";
}

