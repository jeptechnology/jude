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

jude_size_t jude_rtti_field_count(const jude_rtti_t *type)
{
   jude_size_t count = 0;

   if (type)
   {
      const jude_field_t* fields = type->field_list;
      while (fields->tag)
      {
         count++;
         fields++;
      }
   }

   return count;
}

const jude_field_t *jude_rtti_find_field(const jude_rtti_t *type, const char *name)
{
   if (type)
   {
      const jude_field_t* field = type->field_list;
      while (field->tag)
      {
         if (0 == strcmp(field->label, name))
         {
            return field;
         }
         field++;
      }
   }

   return NULL;
}

jude_size_t jude_rtti_bytes_in_field_mask(const jude_rtti_t *type)
{
   jude_size_t number_of_fields = jude_rtti_field_count(type);
   return ((number_of_fields * 2) + 7) / 8;
}

typedef struct
{
   struct
   {
      const jude_rtti_t *type;
      bool has_visited;
   } types[64];
   jude_size_t types_count;
} visit_status;

static jude_size_t get_unvisited(const visit_status *status)
{
   for (jude_size_t index = 0; index < status->types_count; index++)
   {
      if (!status->types[index].has_visited)
      {
         return index;
      }
   }
   return status->types_count;
}

static bool contains(const visit_status *status, const jude_rtti_t* type)
{
   for (jude_size_t index = 0; index < status->types_count; index++)
   {
      if (status->types[index].type == type)
      {
         return true;
      }
   }
   return false;
}

static bool has_visited(const visit_status *status, const jude_rtti_t* type)
{
   for (jude_size_t index = 0; index < status->types_count; index++)
   {
      if (   status->types[index].type == type
          && status->types[index].has_visited)
      {
         return true;
      }
   }
   return false;
}

static bool add_unvisited(visit_status *status, const jude_rtti_t* type)
{
   if (status->types_count >= 32)
   {
      return false;
   }

   for (jude_size_t index = 0; index < status->types_count; index++)
   {
      if (status->types[index].type == type)
      {
         return true; // already in our list of "unvisited" - so don't add again!
      }
   }

   status->types[status->types_count].type = type;
   status->types[status->types_count].has_visited = false;
   status->types_count++;

   return true;
}

static bool jude_rtti_visit_one_type(visit_status *status, const jude_rtti_t *type, jude_rtti_visitor *callback, void *callback_data)
{
   if (!callback(type, callback_data))
   {
      return false;
   }

   for (unsigned index = 0; index < type->field_count; index++)
   {
      const jude_field_t *field = &type->field_list[index];

      if (   jude_field_is_object(field)
         && !contains(status, field->details.sub_rtti)
         && !has_visited(status, field->details.sub_rtti))
      {
         add_unvisited(status, field->details.sub_rtti);
      }
   }

   return true;
}

static bool visit_all(visit_status *status, jude_rtti_visitor *callback, void *callback_data)
{
   jude_size_t unvisted_index = 0;

   unvisted_index = get_unvisited(status);
   while(unvisted_index < status->types_count)
   {
      status->types[unvisted_index].has_visited = true;
      if (!jude_rtti_visit_one_type(status, status->types[unvisted_index].type, callback, callback_data))
      {
         return false;
      }
      unvisted_index = get_unvisited(status);
   }

   return true;
}

bool jude_rtti_visit(const jude_rtti_t *type, jude_rtti_visitor *callback, void *callback_data)
{
   visit_status status;
   status.types_count = 0;
   add_unvisited(&status, type);
   return visit_all(&status, callback, callback_data);
}
