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

// Runtime Type Info - created in auto generated code
typedef struct jude_rtti_t
{
   const char* name;
   const jude_field_t *field_list;
   jude_size_t         field_count;
   jude_size_t         data_size;
} jude_rtti_t;

jude_size_t jude_rtti_field_count(const jude_rtti_t *type);
jude_size_t jude_rtti_bytes_in_field_mask(const jude_rtti_t *type);
const jude_field_t *jude_rtti_find_field(const jude_rtti_t *, const char *name);

typedef bool jude_rtti_visitor(const jude_rtti_t*, void *user_data);
bool jude_rtti_visit(const jude_rtti_t *type,
                     jude_rtti_visitor *callback,
                     void *callback_data);

#ifdef __cplusplus
}
#endif
