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

#define JUDE_BITMASK_SIZE(numOfFields) ((((numOfFields) * 2) + 7) / 8)
#define JUDE_HEADER_DECL(numOfFields) \
      const jude_rtti_t *__rtti;            /* type info about fields for this object                             */\
      jude_size_t __parent_offset;          /* 0 for root resources, take this away from pointer to get to parent */\
      uint8_t     __child_index;            /* index of this object within its parent (if any)                    */\
      jude_id_t   m_id;                     /* unique identifier for this message object                          */\
      uint8_t     __mask[JUDE_BITMASK_SIZE(numOfFields)]  /* note this is actaully variable in length             */\

// Object object
struct jude_object_t
{
   JUDE_HEADER_DECL(1);
};

/* Wire types. Library user needs these only in encoder callbacks. */
typedef enum
{
   PROTOBUF_WT_VARINT = 0,
   PROTOBUF_WT_STRING = 2,

   PROTOBUF_WT_ERROR = -1,
} gpb_wire_type_t;

gpb_wire_type_t get_protobuf_wire_type(jude_type_t type);

#ifdef __cplusplus
}
#endif
