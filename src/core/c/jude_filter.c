/*
 * The MIT License (MIT)
 * Copyright © 2022 James Parker
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <jude/jude_core.h>

const uint8_t AllTouchedByteMask = 0x55;          // 0101b
const uint32_t AllTouchedWordMask = 0x55555555;    // 0101010101010101b
const uint8_t AllChangedByteMask = 0xAA;          // 1010b
const uint32_t AllChangedWordMask = 0xAAAAAAAA;    // 1010101010101010b

/*
 * even bits (0, 2, ...) are for "isset" bits to denote if a field has been touched (i.e. exists)
 * odd bits (1, 3, ...) are for "dirty" bits to denote if a field has changed
 */
#define JUDE_INDEX_TO_TOUCHED_BIT(index)  ((index) << 1)
#define JUDE_INDEX_TO_CHANGED_BIT(index) (((index) << 1) + 1)

static const unsigned number_of_words_in_filter_mask = (sizeof(jude_filter_t) / sizeof(uint32_t));

bool jude_filter_is_empty(const jude_filter_t *lhs)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      if (lhs->words[index] != 0)
      {
         return false;
      }
   }
   return true;
}

bool jude_filter_is_any_changed(const jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      if (filter->words[index] & AllChangedWordMask)
      {
         return true;
      }
   }
   return false;
}

bool jude_filter_is_any_touched(const jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      if (filter->words[index] & AllTouchedWordMask)
      {
         return true;
      }
   }
   return false;
}

bool jude_filter_is_overlapping(const jude_filter_t *lhs, const jude_filter_t *rhs)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      if (lhs->words[index] & rhs->words[index])
      {
         return true; // non-empty intersection
      }
   }
   return false;
}

void jude_filter_clear_all(jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      filter->words[index] = 0;
   }
}

void jude_filter_clear_all_changed(jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      filter->words[index] &= ~AllChangedWordMask;
   }
}

void jude_filter_clear_all_touched(jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      filter->words[index] &= ~AllTouchedWordMask;
   }
}

void jude_filter_fill_all(jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      filter->words[index] = 0xFFFFFFFF;
   }
}

void jude_filter_fill_all_changed(jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      filter->words[index] |= AllChangedWordMask;
   }
}

void jude_filter_fill_all_touched(jude_filter_t *filter)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      filter->words[index] |= AllTouchedWordMask;
   }
}

void jude_filter_and_equals(jude_filter_t *lhs, const jude_filter_t *rhs)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      lhs->words[index] &= rhs->words[index];
   }
}

void jude_filter_or_equals(jude_filter_t *lhs, const jude_filter_t *rhs)
{
   uint32_t index;
   for (index = 0; index < number_of_words_in_filter_mask; index++)
   {
      lhs->words[index] |= rhs->words[index];
   }
}

void jude_filter_set_changed(jude_bitfield_t mask, jude_size_t index, bool is_dirty)
{
   if (is_dirty)
      jude_bitfield_set(mask, JUDE_INDEX_TO_CHANGED_BIT(index));
   else
      jude_bitfield_clear(mask, JUDE_INDEX_TO_CHANGED_BIT(index));
}

void jude_filter_set_touched(jude_bitfield_t mask, jude_size_t index, bool is_set)
{
   if (is_set)
      jude_bitfield_set(mask, JUDE_INDEX_TO_TOUCHED_BIT(index));
   else
      jude_bitfield_clear(mask, JUDE_INDEX_TO_TOUCHED_BIT(index));
}

bool jude_filter_is_changed(jude_const_bitfield_t mask, jude_size_t index)
{
   return jude_bitfield_is_set(mask, JUDE_INDEX_TO_CHANGED_BIT(index));
}

bool jude_filter_is_touched(jude_const_bitfield_t mask, jude_size_t index)
{
   return jude_bitfield_is_set(mask, JUDE_INDEX_TO_TOUCHED_BIT(index));
}

void jude_filter_clear_if_touched_and_mark_changed(jude_bitfield_t mask, jude_size_t index)
{
   if (jude_filter_is_touched(mask, index))
   {
      jude_filter_set_touched(mask, index, false);
      jude_filter_set_changed(mask, index, true);
   }
}
