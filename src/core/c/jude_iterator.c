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

void jude_iterator_reset(jude_iterator_t *iterator)
{
   jude_assert(iterator);
   jude_assert(iterator->object);

   iterator->field_index = 0;
   iterator->current_field = &iterator->object->__rtti->field_list[0];
   iterator->details.data = (char*) iterator->object + iterator->current_field->data_offset;
}

jude_iterator_t jude_iterator_begin(jude_object_t *message)
{
   jude_assert(message != NULL);
   jude_iterator_t iter =
   {
      .object = message
   };
   jude_iterator_reset(&iter);
   return iter;
}

bool jude_iterator_next(jude_iterator_t *iter)
{
   const jude_field_t *prev_field = iter->current_field;

   if (prev_field->tag == 0)
   {
      /* Handle empty message types, where the first field is already the terminator.
       * In other cases, the iter->current_field never points to the terminator. */
      return false;
   }

   iter->current_field++;

   if (iter->current_field->tag == 0)
   {
      /* Wrapped back to beginning, reinitialize */
      jude_iterator_reset(iter);
      return false;
   }
   else
   {
      /* Increment the pointers based on previous field size */
      size_t prev_size = prev_field->data_size;

      if (jude_field_is_array(prev_field))
      {
         /* In static arrays, the data_size tells the size of a single entry and
          * array_size is the number of entries */
         prev_size *= prev_field->array_size;
      }

      iter->field_index++;
      iter->details.data = (char*) iter->details.data + prev_size + iter->current_field->data_offset;
      return true;
   }
}

/* Advance the iterator to the given field index.
 * Returns false when the iterator wraps back to the first field. */
bool jude_iterator_go_to_index(jude_iterator_t *iter, jude_size_t index)
{
   const jude_field_t *start = iter->current_field;

   do
   {
      if (iter->field_index == index)
      {
         /* Found the wanted field */
         return true;
      }

      (void) jude_iterator_next(iter);
   } while (iter->current_field != start);

   iter->field_index = JUDE_UNKNOWN_FIELD_INDEX;

   /* Searched all the way back to start, and found nothing. */
   return false;
}


bool jude_iterator_find(jude_iterator_t *iter, uint32_t tag)
{
   const jude_field_t *start = iter->current_field;

   // NOTE: Normal protobuf does not allow value of 0
   // In cases where we want to specify a mapping using vlaue 0, we must replace
   // with a special value tag of 0xFFFF - this helps in particular with ZigBee mapping
   // from Attribute ID -> jude Tag
   if (tag == 0)
   {
      tag = 0xFFFF;
   }

   do
   {
      if (iter->current_field->tag == tag)
      {
         /* Found the wanted field */
         return true;
      }

      (void) jude_iterator_next(iter);
   } while (iter->current_field != start);

   iter->field_index = JUDE_UNKNOWN_FIELD_INDEX;

   /* Searched all the way back to start, and found nothing. */
   return false;
}

bool jude_iterator_find_by_name(jude_iterator_t *iter, const char *name)
{
   const jude_field_t *start = iter->current_field;

   do
   {
      if (strcmp(iter->current_field->label, (char*) name) == 0)
      {
         /* Found the wanted field */
         return true;
      }

      (void) jude_iterator_next(iter);
   } while (iter->current_field != start);

   /* Searched all the way back to start, and found nothing. */
   return false;
}

bool jude_iterator_is_touched(const jude_iterator_t *iter)
{
   return iter
       && iter->object
       && jude_filter_is_touched(iter->object->__mask, iter->field_index);
}

void jude_iterator_set_touched(jude_iterator_t *iter)
{
   if (iter && iter->object)
   {
      jude_object_mark_field_touched(iter->object, iter->field_index, true);
   }
}

void jude_iterator_clear_touched(jude_iterator_t *iter)
{
   if (iter && iter->object)
   {
      jude_object_mark_field_touched(iter->object, iter->field_index, false);
   }
}

bool jude_iterator_is_changed(const jude_iterator_t *iter)
{
   return iter
      && iter->object
      && jude_filter_is_changed(iter->object->__mask, iter->field_index);
}

void jude_iterator_set_changed(jude_iterator_t *iter)
{
   if (iter && iter->object)
   {
      jude_object_mark_field_changed(iter->object, iter->field_index, true);
   }
}

void jude_iterator_clear_changed(jude_iterator_t *iter)
{
   if (iter && iter->object)
   {
      jude_object_mark_field_changed(iter->object, iter->field_index, false);
   }
}

void* jude_iterator_get_data(jude_iterator_t *iter, jude_size_t array_index)
{
   if (jude_field_is_array(iter->current_field))
   {
      return jude_get_array_data(iter->current_field, iter->details.data, array_index);
   }
   else if (array_index > 0)
   {
      return NULL; // non-array so array_index not 0 does not exist
   }

   return iter->details.data;
}

jude_object_t* jude_iterator_get_subresource(jude_iterator_t *iter, jude_size_t array_index)
{
   if (!jude_iterator_is_subresource(iter))
   {
      return NULL;
   }
   return (jude_object_t*)jude_iterator_get_data(iter, array_index);
}

jude_size_t jude_iterator_get_size(const jude_iterator_t *iter)
{
   return jude_field_get_size(iter->current_field);
}

jude_size_t jude_iterator_get_count(const jude_iterator_t *iter)
{
   if (!iter)
   {
      return 0;
   }

   if (!jude_filter_is_touched(iter->object->__mask, iter->field_index))
   {
      return 0;
   }

   if (!jude_field_is_array(iter->current_field))
   {
      return  1;
   }

   return *jude_iterator_get_count_reference((jude_iterator_t *)iter);
}

// get pointer to the "count" variable (if iterator not pointing at array field, returns NULL)
jude_size_t* jude_iterator_get_count_reference(jude_iterator_t *iter)
{
   if (!iter) return NULL;
   return jude_get_array_count_reference(iter->current_field, iter->details.data);
}

bool jude_iterator_is_array(const jude_iterator_t *iter)
{
   return jude_field_is_array(iter->current_field);
}

bool jude_iterator_is_subresource(const jude_iterator_t *iter)
{
   return jude_field_is_object(iter->current_field);
}

bool jude_iterator_is_string(const jude_iterator_t* iter)
{
   return jude_field_is_string(iter->current_field);
}