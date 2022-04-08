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

#define JUDE_BIT_ARRAY_INDEX(bit)  (uint8_t)(bit >> 3)
#define JUDE_BIT_ARRAY_OFFEST(bit) (uint8_t)(1 << ((uint8_t)((bit) & 7)))

#define JUDE_SET_FIELD_BIT(field, bit)   (((uint8_t *)field)[JUDE_BIT_ARRAY_INDEX(bit)] |=  JUDE_BIT_ARRAY_OFFEST(bit))
#define JUDE_IS_FIELD_BIT(field, bit)    (((uint8_t *)field)[JUDE_BIT_ARRAY_INDEX(bit)] &   JUDE_BIT_ARRAY_OFFEST(bit))
#define JUDE_CLEAR_FIELD_BIT(field, bit) (((uint8_t *)field)[JUDE_BIT_ARRAY_INDEX(bit)] &= ~JUDE_BIT_ARRAY_OFFEST(bit))

void jude_bitfield_set(jude_bitfield_t data, jude_size_t index)
{
   JUDE_SET_FIELD_BIT(data, index);
}

void jude_bitfield_clear(jude_bitfield_t data, jude_size_t index)
{
   JUDE_CLEAR_FIELD_BIT(data, index);
}

void jude_bitfield_clear_all(jude_bitfield_t data, jude_size_t bitWidth)
{
   for (jude_size_t index = 0; index < (bitWidth + 7U) / 8U; index++)
   {
      data[index] = 0;
   }
}

bool jude_bitfield_is_set(jude_const_bitfield_t data, jude_size_t index)
{
   return JUDE_IS_FIELD_BIT(data, index) != 0;
}
