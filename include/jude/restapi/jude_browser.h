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

#include <jude/jude_core.h>
#include "jude_rest_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
   jude_browser_INVALID = 0,
   jude_browser_OBJECT,
   jude_browser_ARRAY,
   jude_browser_FIELD
} jude_browse_node_t;

typedef enum
{
   jude_permission_None,
   jude_permission_Read,
   jude_permission_Write
} jude_permission_t;

typedef struct jude_browser_t
{
   jude_browse_node_t type;
   jude_user_t access_level;
   union
   {
      jude_object_t  *object;
      jude_iterator_t  array; // iterator within the parent
      struct {
         jude_iterator_t iterator;    // iterator within the parent
         jude_size_t     array_index; // if the field is an array we store its array_index here
      } field;
   } x;
   jude_restapi_code_t code;
   const char* remaining_suffix;
} jude_browser_t;

jude_browser_t jude_browser_init(jude_object_t *, jude_user_t access_level);
bool jude_browser_is_valid(jude_browser_t *);
bool jude_browser_is_object(jude_browser_t *);
bool jude_browser_is_array(jude_browser_t *);
bool jude_browser_is_field(jude_browser_t *);
jude_object_t*     jude_browser_get_object(jude_browser_t *);
jude_iterator_t*     jude_browser_get_array(jude_browser_t *);
const jude_field_t*  jude_browser_get_field_type(jude_browser_t *);
void*                jude_browser_get_field_data(jude_browser_t *);

bool jude_browser_follow_path(jude_browser_t *, const char *path_token, jude_permission_t required_permission);
jude_browser_t jude_browser_try_path(jude_object_t* root, const char* fullpath, jude_user_t access_level, jude_permission_t required_permission);

#ifdef __cplusplus
}
#endif

