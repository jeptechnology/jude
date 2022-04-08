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

bool jude_field_is_array(const jude_field_t *field)
{
   return field->array_size != 0;
}

bool jude_field_is_string(const jude_field_t *field)
{
   return field->type == JUDE_TYPE_STRING;
}

bool jude_field_is_numeric(const jude_field_t *field)
{
   switch (field->type)
   {
   case JUDE_TYPE_SIGNED:
   case JUDE_TYPE_UNSIGNED:
   case JUDE_TYPE_BOOL:
   case JUDE_TYPE_ENUM:
   case JUDE_TYPE_BITMASK:
   case JUDE_TYPE_FLOAT:
      return true;
   default:
      return false;
   }
}

bool jude_field_is_object(const jude_field_t *field)
{
   return field->type == JUDE_TYPE_OBJECT;
}

bool jude_field_is_readable(const jude_field_t *field, jude_user_t access_level)
{
   return access_level >= field->permissions.read;
}

bool jude_field_is_writable(const jude_field_t *field, jude_user_t access_level)
{
   return access_level >= field->permissions.write;
}

bool jude_field_is_public_readable(const jude_field_t* field)
{
   return jude_field_is_readable(field, jude_user_Public);
}

bool jude_field_is_public_writable(const jude_field_t* field)
{
   return jude_field_is_writable(field, jude_user_Public);
}

bool jude_field_is_admin_readable(const jude_field_t* field)
{
   return jude_field_is_readable(field, jude_user_Admin);
}

bool jude_field_is_admin_writable(const jude_field_t* field)
{
   return jude_field_is_writable(field, jude_user_Admin);
}

bool jude_field_is_persisted(const jude_field_t *field)
{
   return field->persist;
}

jude_size_t  jude_field_get_size(const jude_field_t *field)
{
   return field->data_size;
}

jude_size_t jude_get_array_count(const jude_field_t *field, const void *pData)
{
   if (field->size_offset)
   {
      return *(jude_size_t*) ((const uint8_t*) pData + field->size_offset);
   }
   return 0;
}

jude_size_t *jude_get_array_count_reference(const jude_field_t *field, const void *pData)
{
   if (field->size_offset)
   {
      return (jude_size_t*) ((const uint8_t*) pData + field->size_offset);
   }

   return NULL;
}

uint8_t *jude_get_array_data(const jude_field_t *field, void *pData, jude_size_t index)
{
   return (uint8_t*) pData + (index * field->data_size);
}

const char* jude_get_string(const jude_field_t *field, void *pData, jude_size_t index)
{
   if (!jude_field_is_string(field))
   {
      return false;
   }

   return (const char*)jude_get_array_data(field, pData, index);
}

jude_size_t jude_get_union_tag(const jude_field_t *field, const void *pData)
{
   if (field->size_offset)
      return *(jude_size_t*) ((const uint8_t*) pData + field->size_offset);

   return 0;
}

void jude_set_union_tag(const jude_field_t *field, const void *pData, jude_size_t tag)
{
   if (field->size_offset)
      *(jude_size_t*) ((const uint8_t*) pData + field->size_offset) = tag;
}
