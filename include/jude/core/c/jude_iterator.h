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

#include "jude_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union
{
   const jude_size_t *array_count;
   jude_object_t     *sub_object;
   void              *data;
} jude_field_details_t;

/* Iterator for jude_field_t list */
struct jude_iterator_t
{
   jude_object_t       *object;        /* Start of the message struct */
   const jude_field_t  *current_field; /* Current position of the iterator */
   jude_field_details_t details;       /* Pointer to current field value */
   unsigned char        field_index;   /* Zero-based index that counts ALL fields */
};

/* Initialize the field iterator structure to beginning.
 * Returns false if the message type is empty. */
jude_iterator_t jude_iterator_begin(jude_object_t *object);

/*
 * Reset *existing* iterator to the start
 */
void jude_iterator_reset(jude_iterator_t *);

/* Advance the iterator to the next field.
 * Returns false when the iterator wraps back to the first field. */
bool jude_iterator_next(jude_iterator_t *);

/* Advance the iterator until it points at a field with the given tag.
 * Returns false if no such field exists. */
bool jude_iterator_find(jude_iterator_t *, uint32_t tag);

/* Advance the iterator until it points at a field with the given label.
 * Returns false if no such field exists. */
bool jude_iterator_find_by_name(jude_iterator_t *, const char *label);

/* Advance the iterator to the given field index.
 * Returns false when the iterator wraps back to the first field. */
bool jude_iterator_go_to_index(jude_iterator_t *, jude_size_t index);

// Is the current field set?
bool jude_iterator_is_touched(const jude_iterator_t *);
void jude_iterator_set_touched(jude_iterator_t *);
void jude_iterator_clear_touched(jude_iterator_t *);

// Is the current field changed?
bool jude_iterator_is_changed(const jude_iterator_t *);
void jude_iterator_set_changed(jude_iterator_t *);
void jude_iterator_clear_changed(jude_iterator_t *);

// type info of current field
bool jude_iterator_is_array(const jude_iterator_t *);
bool jude_iterator_is_subresource(const jude_iterator_t *);
bool jude_iterator_is_string(const jude_iterator_t* );

// get pointer to the data of the current field pointed to by the given iterator
void* jude_iterator_get_data(jude_iterator_t *, jude_size_t array_index);
jude_object_t* jude_iterator_get_subresource(jude_iterator_t *, jude_size_t array_index);

// how big is the current field? (NB: for array fields, this is just the size of one element)
jude_size_t jude_iterator_get_size(const jude_iterator_t *);

// 0 or 1 for optional, count for array fields
jude_size_t jude_iterator_get_count(const jude_iterator_t *);

// get pointer to the "count" variable (if iterator not pointing at array field, returns NULL)
jude_size_t* jude_iterator_get_count_reference(jude_iterator_t *);

#ifdef __cplusplus
}
#endif

