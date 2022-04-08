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

#include <jude/core/cpp/BytesArray.h>

namespace jude
{
   BytesArray::BytesArray(Object& object, jude_size_t fieldIndex)
      : BaseArray(object, fieldIndex)
   {
   }

   bool BytesArray::Add(const uint8_t *value, jude_size_t size)
   {
      return jude_object_insert_bytes_field(m_parent.RawData(), m_fieldIndex, count(), value, size);
   }

   bool BytesArray::Insert(jude_size_t array_index, const uint8_t *value, jude_size_t size)
   {
      return jude_object_insert_bytes_field(m_parent.RawData(), m_fieldIndex, array_index, value, size);
   }

   bool BytesArray::Set(jude_size_t array_index, const uint8_t *value, jude_size_t size)
   {
      return jude_object_set_bytes_field(m_parent.RawData(), m_fieldIndex, array_index, value, size);
   }

   const std::vector<uint8_t> BytesArray::Get(jude_size_t array_index) const
   {
      auto bytes = jude_object_get_bytes_field(m_parent.RawData(), m_fieldIndex, array_index);
      if (bytes == nullptr)
      {
         return std::vector<uint8_t>();
      }
      return std::vector<uint8_t>(bytes->bytes, bytes->bytes + bytes->size);
   }
}
