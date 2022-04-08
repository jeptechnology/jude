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
#include "jude_internal.h"

void jude_object_set_rtti(jude_object_t *object, const jude_rtti_t *rtti);

const jude_rtti_t       *jude_object_get_type        (const jude_object_t *);
const jude_subscriber_t *jude_object_get_subcribers  (const jude_object_t *);
jude_id_t                jude_object_get_id          (const jude_object_t *);
uint8_t                  jude_object_get_child_index (const jude_object_t *);
bool                     jude_object_is_top_level    (const jude_object_t *); // i.e. parent is null
bool                     jude_object_is_deleted      (const jude_object_t *); // i.e. "id" field is not touched
const jude_object_t     *jude_object_get_parent_const(const jude_object_t *);
jude_object_t           *jude_object_get_parent      (      jude_object_t *);
jude_const_bitfield_t    jude_object_get_mask_const  (const jude_object_t *);
jude_bitfield_t          jude_object_get_mask        (      jude_object_t *);
jude_filter_t            jude_object_get_filter      (const jude_object_t *);
jude_size_t              jude_object_count_field     (const jude_object_t *, jude_index_t field_index);
jude_size_t*             jude_object_count_field_ref (      jude_object_t *, jude_index_t field_index);

// markers
bool jude_object_is_changed(const jude_object_t *);
bool jude_object_is_touched(const jude_object_t *);
void jude_object_mark_field_changed(jude_object_t*, jude_index_t field_index, bool changed);
void jude_object_mark_field_touched(jude_object_t*, jude_index_t field_index, bool touched);
void jude_object_clear_change_markers(jude_object_t *);
void jude_object_clear_touch_markers(jude_object_t *);
void jude_object_clear_all(jude_object_t *);
void jude_object_mark_subscriptions_changed(jude_object_t*);

// value field accessors
bool jude_object_insert_value_into_array(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const void *value);
bool jude_object_set_value_in_array     (jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const void *value);
const void* jude_object_get_value_in_array(jude_object_t* object, jude_index_t field_index, jude_index_t array_index);
bool jude_object_remove_value_from_array(jude_object_t *object, jude_index_t field_index, jude_index_t array_index);
void jude_object_clear_array            (jude_object_t *object, jude_index_t field_index);
jude_size_t jude_object_copy_from_array (jude_object_t *object, jude_index_t field_index, void *destination, jude_size_t max_elements);

// bytes field accessors
bool        jude_object_insert_bytes_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const uint8_t *source, jude_size_t source_length);
bool        jude_object_set_bytes_field   (jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const uint8_t *source, jude_size_t source_length);
jude_bytes_array_t* jude_object_get_bytes_field(jude_object_t* object, jude_index_t field_index, jude_index_t array_index);
jude_size_t jude_object_read_bytes_field  (jude_object_t *object, jude_index_t field_index, jude_index_t array_index, uint8_t *destination, jude_size_t destination_length);

// string field accessors
bool        jude_object_insert_string_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const char *source);
bool        jude_object_set_string_field   (jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const char *source);
const char *jude_object_get_string_field   (jude_object_t* object, jude_index_t field_index, jude_index_t array_index);

// subresource field accessors
jude_object_t* jude_object_add_subresource   (jude_object_t *object, jude_index_t field_index, jude_id_t requested_id);
bool             jude_object_remove_subresource(jude_object_t *object, jude_index_t field_index, jude_id_t id);
const jude_object_t *jude_object_find_subresource_const(const jude_object_t *, jude_index_t field_index, jude_id_t id);
jude_object_t       *jude_object_find_subresource      (      jude_object_t *, jude_index_t field_index, jude_id_t id);
jude_size_t            jude_object_count_subresources(jude_object_t *object, jude_index_t field_index);
const jude_object_t* jude_object_get_subresource_at_index(const jude_object_t* object, jude_index_t field_index, jude_index_t array_index);

// operations between two objects - subscriptions are not affected
int  jude_object_compare(const jude_object_t *lhs, const jude_object_t *rhs); // output like memcmp()
void jude_object_overwrite_data(jude_object_t* destination, const jude_object_t* source, bool andClearChangeMarkers); // like memcpy() but subscribers left out
void jude_object_transfer_all(jude_object_t* lhs, jude_object_t* rhs);
bool jude_object_copy_data(jude_object_t *destination, const jude_object_t *source); // returns true on difference
bool jude_object_merge_data(jude_object_t *destination, const jude_object_t *source); // returns true on difference

// validate
bool jude_object_validate_changes(jude_object_t* object, char* error_msg, jude_size_t error_msg_length);

// publish
void jude_object_publish_all_changes(jude_object_t *object);

typedef bool jude_object_visitor_func(jude_iterator_t* iter, void *user_data, bool *request_submessage_start);
bool jude_object_visit_with_callback(
      jude_object_t *src_struct,
      void *user_data,
      jude_object_visitor_func *visitor_callback,
      bool force_repeated_fields_as_single_entity);

#ifdef __cplusplus
}
#endif

