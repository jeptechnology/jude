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
#include <jude/core/c/jude_internal.h>

const jude_rtti_t *jude_object_get_type(const jude_object_t *object)
{
   return object->__rtti;
}

jude_id_t jude_object_get_id(const jude_object_t *object)
{
   return object->m_id;
}

uint8_t jude_object_get_child_index(const jude_object_t *object)
{
   return object->__child_index;
}

const jude_object_t *jude_object_get_parent_const(const jude_object_t *object)
{
   if (object->__parent_offset == 0)
   {
      return NULL;
   }

   return (const jude_object_t *)((char *)object - object->__parent_offset);
}

jude_object_t *jude_object_get_parent(jude_object_t *object)
{
   return (jude_object_t *)jude_object_get_parent_const(object);
}

bool jude_object_is_top_level(const jude_object_t *object)
{
   return object->__parent_offset == 0;
}

bool jude_object_is_deleted(const jude_object_t *object)
{
   if (object == NULL)
   {
      return true;
   }

   /*
   if (jude_object_get_parent_const(object) == NULL)
   {
      return false; // root objects can't be deleted 
   }
   */

   // Note: Root objects are not normally "deletable"
   // But here we check that two conditions that make it clear this top level object has been
   // marked for deletion:
   //    1. The "id" field is not set
   //    2. The "id" field has changed
   return  !jude_filter_is_touched(object->__mask, JUDE_ID_FIELD_INDEX)
         && jude_filter_is_changed(object->__mask, JUDE_ID_FIELD_INDEX);
}

jude_const_bitfield_t jude_object_get_mask_const(const jude_object_t *object)
{
   return object->__mask;
}

jude_bitfield_t jude_object_get_mask(jude_object_t *object)
{
   return object->__mask;
}

jude_filter_t jude_object_get_filter(const jude_object_t *object)
{
   jude_filter_t filter = { 0 };
   unsigned bytes_in_bitmask = jude_rtti_bytes_in_field_mask(object->__rtti);
   for (unsigned index = 0; index < bytes_in_bitmask; index++)
   {
      filter.mask[index] = object->__mask[index];
   }
   return filter;
}

const jude_object_t* jude_object_get_subresource_at_index(const jude_object_t* object, jude_index_t field_index, jude_index_t array_index)
{
   jude_iterator_t iter = jude_iterator_begin(jude_remove_const(object));
   jude_iterator_go_to_index(&iter, field_index);
   if (!jude_iterator_is_subresource(&iter))
   {
      return NULL; // not a sub object type
   }

   // array - we need to find id match
   jude_size_t count = jude_iterator_get_count(&iter);
   if (array_index < count)
   {
      jude_object_t* subresource = (jude_object_t*)jude_iterator_get_data(&iter, array_index);
      if (jude_filter_is_touched(subresource->__mask, 0))
      {
         return subresource;
      }
   }
   return NULL;
}

const jude_object_t *jude_object_find_subresource_const(const jude_object_t *object, jude_index_t field_index, jude_id_t id)
{
   jude_iterator_t iter = jude_iterator_begin(jude_remove_const(object));
   jude_iterator_go_to_index(&iter, field_index);
   if (!jude_iterator_is_subresource(&iter))
   {
      return NULL; // not a sub object type
   }

   if (!jude_iterator_is_array(&iter))
   {
      return (jude_object_t*)jude_iterator_get_data(&iter, 0);
   }

   // array - we need to find id match
   jude_size_t count = jude_iterator_get_count(&iter);
   for (jude_size_t index = 0; index < count; index++)
   {
      jude_object_t* subresource = (jude_object_t*)jude_iterator_get_data(&iter, index);
      if (jude_filter_is_touched(subresource->__mask, 0) && subresource->m_id == id)
      {
         return subresource;
      }
   }
   return NULL; // not implemented yet
}

jude_object_t *jude_object_find_subresource(jude_object_t *object, jude_index_t field_index, jude_id_t id)
{
   return (jude_object_t *)jude_object_find_subresource_const(object, field_index, id);
}

static bool clear_change_markers_visitor(
      jude_iterator_t* iter,
      void *user_data,
      bool *request_submessage_start)
{
   if (request_submessage_start == NULL || !(*request_submessage_start))
   {
      jude_filter_set_changed(iter->object->__mask, iter->field_index, false);
   }
   return true;
}

static bool clear_touched_markers_visitor(jude_iterator_t* iter,
                                          void *user_data,
                                          bool *request_submessage_start)
{
   if (request_submessage_start == NULL || !(*request_submessage_start))
   {
      jude_filter_set_touched(iter->object->__mask, iter->field_index, false);

      // If this is a repeated field, we can get the count variable and reset it to 0
      jude_size_t *arrayCount = jude_iterator_get_count_reference(iter);
      if (arrayCount)
      {
         *arrayCount = 0;
      }
   }

   return true;
}

static bool clear_all_visitor(jude_iterator_t* iter,
                              void *user_data,
                              bool *request_submessage_start)
{
   clear_touched_markers_visitor(iter, user_data, request_submessage_start);
   clear_change_markers_visitor(iter, user_data, request_submessage_start);
   return true;
}

/* Initialize message fields to default values, recursively */
void jude_object_clear_change_markers(jude_object_t *object)
{
   jude_object_visit_with_callback(object, NULL, clear_change_markers_visitor, false);
}

void jude_object_clear_touch_markers(jude_object_t *object)
{
   jude_object_visit_with_callback(object, NULL, clear_touched_markers_visitor, false);
}

void jude_object_clear_all(jude_object_t *object)
{
   jude_object_visit_with_callback(object, NULL, clear_all_visitor, false);
   object->m_id = (jude_size_t)JUDE_INVALID_ID;
}

void jude_object_mark_field_changed(jude_object_t *object, jude_index_t field_index, bool changed)
{
   if (jude_filter_is_changed(object->__mask, field_index) != changed)
   {
      jude_filter_set_changed(object->__mask, field_index, changed);
      jude_object_t* parent = jude_object_get_parent(object);
      if (parent)
      {
         // if this object is "changed" then any parent will be changed too
         jude_object_mark_field_changed(parent, object->__child_index, true);
      }
   }
}

void jude_object_mark_field_touched(jude_object_t *object, jude_index_t field_index, bool touched)
{
   if (jude_filter_is_touched(object->__mask, field_index) != touched)
   {
      jude_filter_set_touched(object->__mask, field_index, touched);

      if (!touched && jude_field_is_object(&object->__rtti->field_list[field_index]))
      {
         // For an object to be cleared, it must be recursively
         jude_object_t* subobject = jude_object_find_subresource(object, field_index, 0);
         if (subobject)
         {
            jude_object_clear_all(subobject);
         }
      }

      if (object->__rtti->field_list[field_index].is_action)
      {
         return; // do nothing else for clearing an action should be benign
      }

      jude_object_t* parent = jude_object_get_parent(object);
      if (parent && touched)
      {
         // if this object is "touched" then any parent will be touched too
         jude_object_mark_field_touched(parent, object->__child_index, true);
      }

      // by changing the "touch" status, we have changed this object
      jude_object_mark_field_changed(object, field_index, true);
   }
}

jude_filter_t jude_object_get_field_mask(const jude_object_t *object)
{
   jude_filter_t field_mask = JUDE_EMPTY_FILTER;
   jude_size_t bytes_in_mask = jude_rtti_bytes_in_field_mask(object->__rtti);
   memcpy(field_mask.mask, object->__mask, bytes_in_mask);
   return field_mask;
}

bool jude_object_is_changed(const jude_object_t *object)
{
   jude_filter_t field_mask = jude_object_get_field_mask(object);
   return jude_filter_is_any_changed(&field_mask);
}

bool jude_object_is_touched(const jude_object_t *object)
{
   jude_filter_t field_mask = jude_object_get_field_mask(object);
   return jude_filter_is_any_touched(&field_mask);
}

/*
 * Similar to memcmp but for messages. This is important because:
 *
 *  - We need to ignore bitmask differences that just show different fields are dirty
 *  - We need to ignore memory differences in fields that are marked unset
 *  - We need to ignore memory differences in repeated fields where the data is not relevant
 */
int jude_object_compare(const jude_object_t *lhs_struct, const jude_object_t *rhs_struct)
{
   static const int LhsGreaterThanRhs = 1;
   static const int LhsLessThanRhs = -1;
   static const int LhsEqualToRhs = 0;

   // trivial cases
   if (lhs_struct == rhs_struct) return LhsEqualToRhs;
   if (lhs_struct == NULL) return LhsLessThanRhs;
   if (rhs_struct == NULL) return LhsGreaterThanRhs;

   int result;
   jude_iterator_t lhs_iter = jude_iterator_begin(jude_remove_const(lhs_struct));
   jude_iterator_t rhs_iter = jude_iterator_begin(jude_remove_const(rhs_struct));

   // iteratre through the fields, use recursion on submessages (and arrays of sub messages!)
   do
   {
      bool lhs_isset = jude_filter_is_touched(lhs_iter.object->__mask, lhs_iter.field_index);
      bool rhs_isset = jude_filter_is_touched(rhs_iter.object->__mask, rhs_iter.field_index);

      if (!lhs_isset && !rhs_isset)
      {
         // neither field set, skip test
         continue;
      }
      else if (lhs_isset && !rhs_isset)
      {
         return LhsGreaterThanRhs;
      }
      else if (!lhs_isset && rhs_isset)
      {
         return LhsLessThanRhs;
      }

      if (lhs_iter.current_field->type == JUDE_TYPE_OBJECT)
      {
         // When comparing protobuf messages, we must treat submessages field in a special way
         // they contain dirty flags that may differ but that should not affect equality
         // Here, we recurse into each sub message (more complicated if it's an aray of submessages!)
         if (!jude_iterator_is_array(&lhs_iter))
         {
            // Recurse into single sub message
            result = jude_object_compare(lhs_iter.details.sub_object, rhs_iter.details.sub_object);
            if (result != 0)
            {
               return result;
            }
         }
         else
         {
            // Recurse into each sub message
            jude_size_t index;
            jude_size_t lhs_count = jude_get_array_count(lhs_iter.current_field, lhs_iter.details.data);
            jude_size_t rhs_count = jude_get_array_count(rhs_iter.current_field, rhs_iter.details.data);

            if (lhs_count < rhs_count)
            {
               return LhsLessThanRhs;
            }
            else if (lhs_count > rhs_count)
            {
               return LhsGreaterThanRhs;
            }

            for (index = 0; index < lhs_count; index++)
            {
               jude_object_t *lhs_subresource = (jude_object_t*) jude_iterator_get_data(&lhs_iter, index);
               jude_object_t *rhs_subresource = (jude_object_t*) jude_iterator_get_data(&rhs_iter, index);

               result = jude_object_compare(lhs_subresource, rhs_subresource);
               if (result != 0)
               {
                  return result;
               }
            }
         }
      }
      else
      {
		  jude_size_t lengthOfDataToCompare = lhs_iter.current_field->data_size;

         if (jude_iterator_is_array(&lhs_iter))
         {
            jude_size_t lhs_count = jude_iterator_get_count(&lhs_iter);
            jude_size_t rhs_count = jude_iterator_get_count(&rhs_iter);

            if (lhs_count < rhs_count)
            {
               return LhsLessThanRhs;
            }
            else if (lhs_count > rhs_count)
            {
               return LhsGreaterThanRhs;
            }
            else
            {
               lengthOfDataToCompare = lhs_count * jude_field_get_size(lhs_iter.current_field);
            }
         }

         result = memcmp(lhs_iter.details.data, rhs_iter.details.data, lengthOfDataToCompare);
         if (result != 0)
         {
            return result;
         }
      }

   } while (jude_iterator_next(&lhs_iter) && jude_iterator_next(&rhs_iter));

   return LhsEqualToRhs;
}

static bool copy_object(jude_object_t *lhs, jude_object_t *rhs, bool copy_rhs_changes_only);

static bool copy_field(jude_iterator_t *lhs, jude_iterator_t *rhs, bool copy_rhs_changes_only)
{
   if (copy_rhs_changes_only && !jude_iterator_is_changed(rhs))
   {
      return false;
   }

   bool lhs_isset = jude_iterator_is_touched(lhs);
   bool rhs_isset = jude_iterator_is_touched(rhs);

   if (!rhs_isset)
   {
      if (!lhs_isset)
      {
         // not present in either -> nothing to do
         return false;
      }

      // in lhs but not in rhs -> remove from lhs
      jude_iterator_clear_touched(lhs); // no longer *touched* (i.e. null)
      jude_iterator_set_changed(lhs);  // *changed* to null
      return true;
   }

   bool any_change = false;

   // reach here - and rhs is set so we need to set lhs
   jude_iterator_set_touched(lhs);

   // for repeated fields, we need to ensure we fix up the counts
   jude_size_t rhs_count = jude_iterator_get_count(rhs);
   jude_size_t *lhs_count = jude_iterator_get_count_reference(lhs);
   if (lhs_count && *lhs_count != rhs_count)
   {
      *lhs_count = rhs_count;
      jude_iterator_set_changed(lhs);
      any_change = true;
   }

   // for atomic fields, we can just memcmp and memcpy
   if (!jude_iterator_is_subresource(lhs))
   {
      jude_size_t data_size = jude_iterator_get_size(rhs) * rhs_count;

      any_change |= !lhs_isset;
      any_change |= memcmp(jude_iterator_get_data(lhs, 0),
                           jude_iterator_get_data(rhs, 0),
                           data_size);
      if (any_change)
      {
         jude_iterator_set_changed(lhs);
         memcpy(jude_iterator_get_data(lhs, 0), jude_iterator_get_data(rhs, 0), data_size);
      }

      return any_change;
   }

   // for subresource fields, we need to recurse into each element
   // NOTE: this still works for single sub objects - there will just be a count of one!
   for (jude_index_t array_index = 0; array_index < rhs_count; array_index++)
   {
      // subresources require recursive step here
      if (copy_object(jude_iterator_get_subresource(lhs, array_index),
                      jude_iterator_get_subresource(rhs, array_index),
                      copy_rhs_changes_only))
      {
         jude_iterator_set_changed(lhs); // make a note if changed
         any_change = true;
      }
   }

   return any_change;
}

static bool copy_object(jude_object_t *lhs, jude_object_t *rhs, bool copy_rhs_changes_only)
{
   jude_assert(lhs);
   jude_assert(rhs);
   jude_assert(lhs->__rtti == rhs->__rtti);

   jude_iterator_t lhs_iter = jude_iterator_begin(lhs);
   jude_iterator_t rhs_iter = jude_iterator_begin(jude_remove_const(rhs));

   // iteratre through the fields, use recursion on submessages (and arrays of sub messages!)
   bool any_changes = false;

   do
   {
      any_changes |= copy_field(&lhs_iter, &rhs_iter, copy_rhs_changes_only);

   } while (jude_iterator_next(&lhs_iter) && jude_iterator_next(&rhs_iter));

   return any_changes;
}

jude_size_t jude_object_count_field(const jude_object_t *object, jude_index_t field_index)
{
   jude_iterator_t iter = jude_iterator_begin(jude_remove_const(object));
   if (!jude_iterator_go_to_index(&iter, field_index)) return 0;
   return jude_iterator_get_count(&iter);
}

jude_size_t* jude_object_count_field_ref (jude_object_t *object, jude_index_t field_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index)) return NULL;
   return jude_iterator_get_count_reference(&iter);
}

bool jude_object_visit_with_callback(
      jude_object_t *object,
      void *user_data,
      jude_object_visitor_func *visitor_callback,
      bool force_repeated_fields_as_single_entity)
{
   jude_iterator_t iter = jude_iterator_begin(object);

   // iteratre through the fields, use recursion on submessages (and arrays of sub messages!)
   do
   {
      if (jude_field_is_object(iter.current_field))
      {
         // first call the visitor with the "start of submessage"
         bool requestStartSubmessage = true;
         if (!visitor_callback(&iter, user_data, &requestStartSubmessage))
            return false;

         // If we are told to skip the sub message then continue to next field
         if (!requestStartSubmessage)
         {
            continue;
         }

         bool should_continue = true;

         // Note that some visitors (e.g. JsonSchema generation)
         // Do not want to trawl through each item in a repeasted list
         // They just want to visit the "repeated" field once
         if (!jude_field_is_array(iter.current_field))
         {
            // Recurse into single sub message
            should_continue = jude_object_visit_with_callback(
                  iter.details.sub_object, user_data, visitor_callback,
                  force_repeated_fields_as_single_entity);
         }
         else if (force_repeated_fields_as_single_entity)
         {
            // Recurse only into first single sub message in repeated array
            jude_size_t index = 0;
            jude_object_t *element_data = (jude_object_t*) jude_iterator_get_data(&iter, index);

            should_continue = jude_object_visit_with_callback(element_data,
                                                              user_data, visitor_callback,
                                                              force_repeated_fields_as_single_entity);
         }
         else
         {
            // Recurse into each sub message
            for (jude_size_t index= 0; index < jude_iterator_get_count(&iter); index++)
            {
               jude_object_t *element_data = jude_iterator_get_subresource(&iter, index);
               should_continue = jude_object_visit_with_callback(element_data,
                                                                 user_data,
                                                                 visitor_callback,
                                                                 force_repeated_fields_as_single_entity);
               if (!should_continue)
               {
                  break;
               }
            }
         }

         if (!should_continue)
         {
            break;
         }
      }

      // Finally execute the visitor callback for this current member of the structure
      bool startOfSubmessage = false;
      if (!visitor_callback(&iter, user_data, &startOfSubmessage))
         return false;

   } while (jude_iterator_next(&iter));

   return true;
}


void* jude_object_get_data(jude_object_t *object, uint8_t field_index, jude_index_t array_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return NULL; // could not find index
   }

   return jude_iterator_get_data(&iter, array_index);
}

static void jude_object_set_type_info(
      jude_object_t *parent,
      jude_object_t *child,
      const jude_rtti_t *type,
      uint8_t childIndex)
{
   child->__child_index = childIndex;
   child->__parent_offset = (jude_size_t)((uint8_t*)child - (uint8_t*)parent);
   child->__rtti = type;
   jude_iterator_t iterator = jude_iterator_begin(child);

   do
   {
	   jude_filter_set_changed(child->__mask, iterator.field_index, false);
	   jude_filter_set_touched(child->__mask, iterator.field_index, false);
	   if (!jude_iterator_is_subresource(&iterator))
      {
         continue;
      }

      if (!jude_iterator_is_array(&iterator))
      {
         jude_object_set_type_info(child, iterator.details.sub_object, iterator.current_field->details.sub_rtti, iterator.field_index);
         continue;
      }

      for (int arrayIndex = 0; arrayIndex < iterator.current_field->array_size; arrayIndex++)
      {
         jude_object_t* subMessageElement = (jude_object_t*)jude_iterator_get_data(&iterator, arrayIndex);
         jude_object_set_type_info(child, subMessageElement, iterator.current_field->details.sub_rtti, iterator.field_index);
      }

   } while (jude_iterator_next(&iterator));
}


void jude_object_set_rtti(jude_object_t *object, const jude_rtti_t *rtti)
{
   jude_object_set_type_info(object, object, rtti, 0);
}

void jude_object_transfer_all(jude_object_t* lhs, jude_object_t* rhs)
{
   jude_assert(lhs);
   jude_assert(rhs);
   jude_assert(lhs->__rtti == rhs->__rtti);

   if (lhs != rhs)
   {
      memcpy(lhs, rhs, lhs->__rtti->data_size);
      jude_object_clear_change_markers(rhs);
   }
}

void jude_object_overwrite_data(jude_object_t *lhs, const jude_object_t *rhs, bool andClearChangeMarkers)
{
   jude_assert(lhs);
   jude_assert(rhs);
   jude_assert(lhs->__rtti == rhs->__rtti);

   if (lhs != rhs)
   {
      // Copy everything other than the initial type information, and parent/child data which is already set and should not be changed
      const size_t dataSizeWithoutHeader = (lhs->__rtti->data_size - offsetof(jude_object_t, m_id));
      memcpy(&lhs->m_id, &rhs->m_id, dataSizeWithoutHeader);

      if (andClearChangeMarkers)
      {
         jude_object_clear_change_markers(lhs);
      }
   }
}

bool jude_object_merge_data(jude_object_t *destination, const jude_object_t *source)
{
   return copy_object(destination, jude_remove_const(source), /* deltas_only = */ true);
}

bool jude_object_copy_data(jude_object_t* destination, const jude_object_t* source)
{
   return copy_object(destination, jude_remove_const(source), /* deltas_only = */ false);
}

static bool jude_object_find_and_check_array_has_room(jude_iterator_t *iter, jude_index_t field_index)
{
   if (  !jude_iterator_go_to_index(iter, field_index)
      || !jude_iterator_is_array(iter)
      || !(jude_iterator_get_count(iter) < iter->current_field->array_size))
   {
      return false;
   }

   return true;
}

// generic field accessors
bool jude_object_insert_value_into_array(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const void *value)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_object_find_and_check_array_has_room(&iter, field_index))
   {
      return false;
   }

   jude_size_t* count = jude_iterator_get_count_reference(&iter);
   if (!jude_iterator_is_touched(&iter))
   {
      // reset count in case we were not set!
      *count = 0;
   }

   if (array_index > *count)
   {
      return false; // trying to insert beyond last element
   }
   else if (array_index < *count)
   {
      // shift following elements forwards...
      size_t elements_to_shift = *count - array_index;
      void *source      = jude_iterator_get_data(&iter, array_index);
      void *destination = (uint8_t*)source + jude_iterator_get_size(&iter);
      memmove(destination, source, elements_to_shift * jude_iterator_get_size(&iter));
   }
   // copy value into place
   if (value)
   {
      memcpy(jude_iterator_get_data(&iter, array_index), value, jude_iterator_get_size(&iter));
   }
   // increment count
   (*count)++;
   // mark as changed and touched
   jude_iterator_set_touched(&iter);
   jude_iterator_set_changed(&iter);
   return true;
}

const void* jude_object_get_value_in_array(jude_object_t* object, jude_index_t field_index, jude_index_t array_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (  !jude_iterator_go_to_index(&iter, field_index)
      || jude_iterator_get_count(&iter) <= array_index
      )
   {
      return NULL;
   }

   return jude_iterator_get_data(&iter, array_index);
}

bool jude_object_set_value_in_array(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const void *value)
{
   jude_iterator_t iter = jude_iterator_begin(object);

   if (  !jude_iterator_go_to_index(&iter, field_index)
      || (jude_iterator_is_array(&iter) && jude_iterator_get_count(&iter) <= array_index)
      )
   {
      return false;
   }

   // is this a change?
   if (!jude_iterator_is_touched(&iter) || (0 != memcmp(jude_iterator_get_data(&iter, array_index), value, jude_iterator_get_size(&iter))))
   {
      jude_iterator_set_touched(&iter);
      jude_iterator_set_changed(&iter);

      // copy value into place
      memcpy(jude_iterator_get_data(&iter, array_index), value, jude_iterator_get_size(&iter));
   }

   return true;
}


bool jude_object_remove_value_from_array(jude_object_t *object, jude_index_t field_index, jude_index_t array_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (  !jude_iterator_go_to_index(&iter, field_index)
      || !jude_iterator_is_array(&iter)
      || !(jude_iterator_get_count(&iter) >= array_index))
   {
      return false;
   }

   jude_size_t* count = jude_iterator_get_count_reference(&iter);
   if (array_index + 1 < *count)
   {
      // shift following elements backwards...
      jude_size_t elements_to_shift = *count - (array_index + 1);
      void *destination = jude_iterator_get_data(&iter, array_index);
      void *source      = (uint8_t*)destination + jude_iterator_get_size(&iter);
      memmove(destination, source, elements_to_shift * jude_iterator_get_size(&iter));
   }

   // decrement count
   (*count)--;
   // mark as changed and touched
   jude_iterator_set_touched(&iter);
   jude_iterator_set_changed(&iter);
   return true;
}

void jude_object_clear_array(jude_object_t *object, jude_index_t field_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index)
      || !jude_iterator_is_array(&iter))
   {
      return;
   }

   // only bother to clear if already set
   if (jude_iterator_is_touched(&iter))
   {
      jude_size_t* count = jude_iterator_get_count_reference(&iter);
      if (count)
      {
         *count = 0;
      }
      jude_iterator_clear_touched(&iter);
      jude_iterator_set_changed(&iter);
   }
}

jude_size_t jude_object_copy_from_array(jude_object_t *object, jude_index_t field_index, void *destination, jude_size_t max_elements)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (  !jude_iterator_go_to_index(&iter, field_index)
      || !jude_iterator_is_array(&iter))
   {
      return 0;
   }

   jude_size_t count = jude_iterator_get_count(&iter);
   jude_size_t copy_count = count < max_elements ? count : max_elements;

   memcpy(destination, jude_iterator_get_data(&iter, 0), copy_count * jude_iterator_get_size(&iter));

   return copy_count;
}

jude_bytes_array_t* jude_object_get_bytes_field(jude_object_t* object, jude_index_t field_index, jude_index_t array_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return NULL;
   }

   jude_size_t count = jude_iterator_get_count(&iter);
   if (count <= array_index)
   {
      return NULL; // array_index out of bounds
   }

   return (jude_bytes_array_t*)jude_iterator_get_data(&iter, array_index);
}

jude_size_t jude_object_read_bytes_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, uint8_t *destination, jude_size_t destination_length)
{
   jude_bytes_array_t* data = jude_object_get_bytes_field(object, field_index, array_index);
   if (data == NULL)
   {
      return 0;
   }

   jude_size_t copy_count = data->size < destination_length ? data->size : destination_length;
   memcpy(destination, data->bytes, copy_count);
   return copy_count;
}

static bool bytes_copy_and_change_check(uint8_t* destination, const uint8_t* source, jude_size_t *length, const jude_iterator_t* iter)
{
   bool has_changed = false;

   jude_size_t max_length = jude_iterator_get_size(iter) - offsetof(jude_bytes_array_t, bytes);

   if (max_length < *length)
   {
      jude_handle_string_overflow(iter->object, iter->current_field->label);
      *length = max_length;
   }
   
   jude_size_t countdown = *length;

   while (countdown)
   {
      has_changed |= (*destination != *source);
      *destination = *source;
      destination++;
      source++;
      countdown--;
   }

   return has_changed;
}

bool jude_object_set_bytes_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const uint8_t *source, jude_size_t source_length)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return false;
   }

   bool was_set = jude_iterator_is_touched(&iter);
   jude_iterator_set_touched(&iter);

   jude_size_t count = jude_iterator_get_count(&iter);
   if (count <= array_index)
   {
      return false; // array_index out of bounds
   }

   jude_bytes_array_t *data = (jude_bytes_array_t*)jude_iterator_get_data(&iter, array_index);

   // copy and check for change
   bool is_changed = bytes_copy_and_change_check(data->bytes, source, &source_length, &iter);
   if (data->size != source_length)
   {
      data->size = source_length;
      is_changed = true;   // length changed => changed!
   }

   if (!was_set || is_changed)
   {
      jude_iterator_set_changed(&iter);
   }
   return true;
}

bool jude_object_insert_bytes_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const uint8_t *source, jude_size_t source_length)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return false;
   }

   jude_size_t max_length = jude_iterator_get_size(&iter) - offsetof(jude_bytes_array_t, bytes);
   if (source_length <= max_length)
   {
      return false;
   }

   // insert "null" just to shift everything up
   if (!jude_object_insert_value_into_array(object, field_index, array_index, 0))
   {
      return false;
   }

   // set the new slot
   return jude_object_set_bytes_field(object, field_index, array_index, source, source_length);
}

bool jude_object_insert_string_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const char *source)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return false;
   }

   // insert "null" just to shift everything up
   if (!jude_object_insert_value_into_array(object, field_index, array_index, NULL))
   {
      return false;
   }

   jude_size_t max_length = jude_iterator_get_size(&iter);
   char *destination = (char*)jude_iterator_get_data(&iter, array_index);
   if (source)
   {
      strncpy(destination, source, max_length - 1);
   }
   destination[max_length - 1] = 0;
   return true;
}

static bool string_copy_and_change_check(char* destination, const char* source, jude_size_t max, const jude_iterator_t* iter)
{
   bool has_changed = false;

   if (destination == source)
   {
      return false;
   }

   while (*source && max)
   {
      has_changed |= (*destination != *source);
      *destination = *source;
      destination++;
      source++;
      max--;
   }

   if (max == 0 && *source != '\0')
   {
      jude_handle_string_overflow(iter->object, iter->current_field->label);
      has_changed = true; // overflow always triggers change
   }
   
   if (*destination != '\0')
   {
      *destination = '\0';
      has_changed = true; // we must have changed length
   }

   return has_changed;
}

bool jude_object_set_string_field(jude_object_t *object, jude_index_t field_index, jude_index_t array_index, const char *source)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return false;
   }

   bool was_set = false;
   jude_size_t count = jude_iterator_get_count(&iter);
   if (jude_iterator_is_array(&iter))
   {
      if (jude_iterator_get_count(&iter) <= array_index)
      {
         return false; // array_index out of bounds
      }
      was_set = true;
   }
   else
   {
      was_set = (count == 1);
   }

   jude_size_t max_length = jude_iterator_get_size(&iter);
   char *destination = (char*)jude_iterator_get_data(&iter, array_index);
   if (!source)
   {
      if (was_set)
      {
         // a NULL source should clear this field
         jude_iterator_clear_touched(&iter);
         jude_iterator_set_changed(&iter);
         destination[0] = '\0';
      }
   }
   else
   {
      // copy and check for change
      bool is_changed = string_copy_and_change_check(destination, source, max_length - 1, &iter);
      jude_iterator_set_touched(&iter);
      if (!was_set || is_changed)
      {
         jude_iterator_set_changed(&iter);
      }
   }

   return true;
}

const char* jude_object_get_string_field(jude_object_t* object, jude_index_t field_index, jude_index_t array_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (!jude_iterator_go_to_index(&iter, field_index))
   {
      return NULL;
   }

   bool is_set = false;
   jude_size_t count = jude_iterator_get_count(&iter);
   if (jude_iterator_is_array(&iter))
   {
      if (jude_iterator_get_count(&iter) <= array_index)
      {
         return NULL; // array_index out of bounds
      }
      is_set = true;
   }
   else
   {
      is_set = (count == 1);
   }

   if (!is_set)
   {
      return NULL;
   }

   char* destination = (char*)jude_iterator_get_data(&iter, array_index);
   return destination;
}


jude_object_t *jude_object_add_subresource(jude_object_t *object, jude_index_t field_index, jude_id_t requested_id)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (   !jude_iterator_go_to_index(&iter, field_index)
       || !jude_iterator_is_subresource(&iter)
       || !jude_iterator_is_array(&iter))
   {
      return NULL; // bad index or not a subresource array
   }

   jude_size_t* count = jude_iterator_get_count_reference(&iter);
   jude_object_t* free_slot = NULL;
   for (jude_size_t index = 0; index < *count; index++)
   {
      jude_object_t* subresource = (jude_object_t*)jude_iterator_get_data(&iter, index);
      if (!jude_filter_is_touched(subresource->__mask, 0))
      {
         if (!free_slot) free_slot = subresource;
         continue;
      }

      if (requested_id == subresource->m_id)
      {
         return NULL; // cannot fulfil requested id - it already exists
      }
   }

   if (!free_slot)
   {
      // array has no free slots with empty id - lets see if we can extend...
      if(jude_iterator_get_count(&iter) >= iter.current_field->array_size)
      {
         return NULL; // no more room left in static subresource array
      }
      free_slot = (jude_object_t*)jude_iterator_get_data(&iter, *count);
      (*count)++;
   }

   jude_object_clear_all(free_slot);

   if (requested_id != JUDE_AUTO_ID)
   {
      free_slot->m_id = requested_id;
   }
   else
   {
      free_slot->m_id = jude_generate_uuid();
   }

   // free_slot will now be set up as the newly occupied slot
   jude_object_mark_field_touched(free_slot, 0, true);
   return free_slot;
}

bool jude_object_remove_subresource(jude_object_t *object, jude_index_t field_index, jude_id_t id)
{
   jude_object_t* subresource = jude_object_find_subresource(object, field_index, id);
   if (!subresource)
   {
      return false;
   }
   jude_object_clear_all(subresource);
   return true;
}

jude_size_t jude_object_count_subresources(jude_object_t *object, jude_index_t field_index)
{
   jude_iterator_t iter = jude_iterator_begin(object);
   if (   !jude_iterator_go_to_index(&iter, field_index)
       || !jude_iterator_is_subresource(&iter)
       || !jude_iterator_is_array(&iter))
   {
      return 0; // bad index or not a subresource array
   }

   jude_size_t count = jude_iterator_get_count(&iter);
   jude_size_t slots_filled = 0;
   for (jude_size_t index = 0; index < count; index++)
   {
      jude_object_t* subresource = (jude_object_t*)jude_iterator_get_data(&iter, index);
      if (jude_filter_is_touched(subresource->__mask, 0))
      {
         slots_filled++;
      }
   }
   return slots_filled;
}
