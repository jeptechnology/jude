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

#define JUDE_ENUM_SIZE_SAFTEY_NET 1024

jude_size_t jude_enum_count(const jude_enum_map_t *map)
{
   jude_size_t size = 0;

   while (map->name && size < JUDE_ENUM_SIZE_SAFTEY_NET)
   {
      size++;
      map++;
   }

   return size;
}

jude_enum_value_t jude_enum_get_value(const jude_enum_map_t *map, const char *name)
{
   const jude_enum_value_t *value = jude_enum_find_value(map, name);
   if (!value)
   {
      jude_fatal("invalid enum value");
   }
   return *value;
}

const jude_enum_value_t *jude_enum_find_value(const jude_enum_map_t *map, const char *name)
{
   while (map && map->name)
   {
      if (strcmp(map->name, name) == 0)
      {
         return &map->value;
      }
      map++;
   }

   return NULL;
}

const char *jude_enum_find_string(const jude_enum_map_t *map, jude_enum_value_t value)
{
   while (map && map->name)
   {
      if (map->value == value)
      {
         return map->name;
      }
      map++;
   }

   return NULL;
}

const char *jude_enum_find_description(const jude_enum_map_t *map, jude_enum_value_t value)
{
   while (map && map->name)
   {
      if (map->value == value)
      {
         return map->description;
      }
      map++;
   }

   return NULL;
}

bool jude_enum_contains_value(const jude_enum_map_t *map, jude_enum_value_t value)
{
   return jude_enum_find_string(map, value) != NULL;
}

