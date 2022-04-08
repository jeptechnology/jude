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
#include "jude_enum.h"
#include "jude_rtti.h"

/* Field Definition Structure */
struct jude_field_t
{
   const char  *label; /* text label used in JSON wire format instead of tag id */
   const char  *description; /* (optional) Used for generated documentation such as Swagger */
   jude_index_t  tag;
   jude_index_t  index;
   jude_type_t  type;
   jude_size_t  data_offset; /* Offset of field data, relative to previous field. */
   jude_ssize_t size_offset; /* Offset of array size or has-boolean, relative to data */
   jude_size_t  data_size;   /* Data size in bytes for a single item */
   jude_size_t  array_size;  /* Maximum number of entries in array */
   bool         persist;     /* Is this field persisted? */
   bool         always_notify; /* set the "is changed" flag even if only touched to same value. */
   bool         is_action;   /* Is this field an action? (if so, we auto remove after change notification) */
   struct {
      jude_user_t  read;   /* Is this field readable from external API? */
      jude_user_t  write;  /* Is this field writable from external API? */
   } permissions;
   
   int64_t min; /* relevant for integer fields - used in docs */
   int64_t max; /* relevant for integer fields - used in docs */

   /* Field definitions for submessage
    * OR default value for all other non-array, non-callback types
    * If null, then field will zeroed. */
   union
   {
      const void            *default_data;
      const jude_rtti_t     *sub_rtti;
      const jude_enum_map_t *enum_map;
   } details;
};

bool jude_field_is_array(const jude_field_t *);
bool jude_field_is_string(const jude_field_t *);
bool jude_field_is_numeric(const jude_field_t *);
bool jude_field_is_object(const jude_field_t *);
bool jude_field_is_persisted(const jude_field_t*);

// Access control
bool jude_field_is_readable(const jude_field_t *, jude_user_t access_level);
bool jude_field_is_writable(const jude_field_t *, jude_user_t access_level);
bool jude_field_is_public_readable(const jude_field_t* field);
bool jude_field_is_public_writable(const jude_field_t* field);
bool jude_field_is_admin_readable(const jude_field_t* field);
bool jude_field_is_admin_writable(const jude_field_t* field);

jude_size_t  jude_field_get_size(const jude_field_t *);

jude_size_t  jude_get_array_count(const jude_field_t *field, const void *pData);
jude_size_t *jude_get_array_count_reference(const jude_field_t *field, const void *pData);
uint8_t     *jude_get_array_data(const jude_field_t *field, void *pData, jude_size_t index);
jude_size_t  jude_get_union_tag(const jude_field_t *field, const void *pData);
void         jude_set_union_tag(const jude_field_t *field, const void *pData, jude_size_t tag);
const char*  jude_get_string(const jude_field_t *field, void *pData, jude_size_t array_index);


#ifdef __cplusplus
}
#endif
