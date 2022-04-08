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

#include <jude/core/c/jude_internal.h>

gpb_wire_type_t get_protobuf_wire_type(jude_type_t type)
{
   switch (type)
   {
   case JUDE_TYPE_BOOL:
   case JUDE_TYPE_SIGNED:
   case JUDE_TYPE_UNSIGNED:
   case JUDE_TYPE_FLOAT:
   case JUDE_TYPE_ENUM:
   case JUDE_TYPE_BITMASK:
      return PROTOBUF_WT_VARINT;

   case JUDE_TYPE_STRING:
   case JUDE_TYPE_BYTES:
   case JUDE_TYPE_OBJECT:
      return PROTOBUF_WT_STRING;

   default:
      return PROTOBUF_WT_ERROR;
   }
}
