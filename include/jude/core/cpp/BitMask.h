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
#include <jude/core/cpp/Object.h>

namespace jude
{

class BitMask
{
   const jude_bitmask_map_t &m_map;
   Object                    m_object;
   jude_size_t               m_fieldIndex;
   jude_size_t               m_size;   // number of bytes in this bitfield
   uint8_t                  *m_data;


public:
   BitMask(const jude_bitmask_map_t &map,
           Object        &object,
           jude_size_t    fieldIndex,
           jude_size_t    arrayIndex = 0);

   bool IsSet() const;   // is this bitfield even set?
   bool IsEmpty() const; // is this bitfield not set or set and empty?

   bool IsBitSet(jude_size_t bit) const;
   void ClearBit(jude_size_t bit);
   void ClearAll();
   void SetBit(jude_size_t bit);

   bool IsBitSet(const char *name) const;
   void ClearBit(const char* name);
   void SetBit(const char* name);
};

}

