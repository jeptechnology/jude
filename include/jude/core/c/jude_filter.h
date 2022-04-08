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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "jude_common.h"
#include "jude_bitfield.h"

#define BYTES_REQUIRED_FOR_BITMASK_OF_SIZE(bits) ((bits + 7)  / 8)
#define WORDS_REQUIRED_FOR_BITMASK_OF_SIZE(bits) ((bits + 31) / 32)

union jude_filter_t
{
   uint32_t words[WORDS_REQUIRED_FOR_BITMASK_OF_SIZE(JUDE_MAX_FIELDS_PER_MESSAGE * 2)]; // note: x2 because we need "touched" and "changed" state for each field
   uint8_t  mask [BYTES_REQUIRED_FOR_BITMASK_OF_SIZE(JUDE_MAX_FIELDS_PER_MESSAGE * 2)];
};

#define JUDE_NULL_FILTER  NULL
#define JUDE_EMPTY_FILTER { { 0 } }

void jude_filter_clear_all(jude_filter_t *);
void jude_filter_clear_all_changed(jude_filter_t *);
void jude_filter_clear_all_touched(jude_filter_t *);

void jude_filter_fill_all(jude_filter_t *);
void jude_filter_fill_all_changed(jude_filter_t *);
void jude_filter_fill_all_touched(jude_filter_t *);

bool jude_filter_is_empty(const jude_filter_t *);
bool jude_filter_is_any_changed(const jude_filter_t *);
bool jude_filter_is_any_touched(const jude_filter_t *);

bool jude_filter_is_overlapping(const jude_filter_t *lhs, const jude_filter_t *rhs);
void jude_filter_and_equals(jude_filter_t *lhs, const jude_filter_t *rhs);
void jude_filter_or_equals(jude_filter_t *lhs, const jude_filter_t *rhs);

void jude_filter_set_changed(jude_bitfield_t mask, jude_size_t index, bool is_set);
void jude_filter_set_touched(jude_bitfield_t mask, jude_size_t index, bool is_set);

bool jude_filter_is_changed(jude_const_bitfield_t mask, jude_size_t index);
bool jude_filter_is_touched(jude_const_bitfield_t mask, jude_size_t index);

void jude_filter_clear_if_touched_and_mark_changed(jude_bitfield_t mask, jude_size_t index);

#ifdef __cplusplus
}
#endif

