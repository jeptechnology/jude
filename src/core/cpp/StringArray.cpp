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

#include <jude/core/cpp/StringArray.h>

namespace jude
{
   StringArray::StringArray(Object& object, jude_size_t fieldIndex)
      : BaseArray(object, fieldIndex)
   {
   }

   bool StringArray::Add(const char *value)
   {
      return jude_object_insert_string_field(m_parent.RawData(), m_fieldIndex, count(), value);
   }

   bool StringArray::Insert(jude_size_t array_index, const char *value)
   {
      return jude_object_insert_string_field(m_parent.RawData(), m_fieldIndex, array_index, value);
   }

   bool StringArray::Set(jude_size_t array_index, const char *value)
   {
      return jude_object_set_string_field(m_parent.RawData(), m_fieldIndex, array_index, value);
   }

   const char *StringArray::Get(jude_size_t array_index) const
   {
      return jude_object_get_string_field(m_parent.RawData(), m_fieldIndex, array_index);
   }
}
