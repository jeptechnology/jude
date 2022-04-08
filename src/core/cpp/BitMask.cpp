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

#include <jude/core/cpp/BitMask.h>

namespace jude 
{

   BitMask::BitMask(const jude_bitmask_map_t& map,
                    Object&        object,
                    jude_size_t    fieldIndex,
                    jude_size_t    arrayIndex)
      : m_map(map)
      , m_object(object)
      , m_fieldIndex(fieldIndex)
   {
      jude_iterator_t iter = jude_iterator_begin(m_object.RawData());
      
      if (  !jude_iterator_go_to_index(&iter, fieldIndex)
         || (jude_iterator_is_array(&iter) && (jude_iterator_get_count(&iter) <= arrayIndex))
         )
      {
         m_data = nullptr;
         m_size = 0;
      }
      else
      {
         m_data = (uint8_t*)jude_iterator_get_data(&iter, arrayIndex);
         m_size = jude_iterator_get_size(&iter);
      }
   }

   bool BitMask::IsSet() const
   {
      return m_object.Has(m_fieldIndex);
   }

   bool BitMask::IsEmpty() const
   {
      if (!IsSet() || !m_data)
      {
         return true;
      }

      switch (m_size)
      {
      case 1:
         return *m_data == 0;
      case 2:
         return *(uint16_t*)m_data == 0;
      case 4:
         return *(uint32_t*)m_data == 0;
      case 8:
         return *(uint64_t*)m_data == 0LL;
      }

      jude_debug("Unexpected size of bit field: %d", m_size);

      return true;
   }

   bool BitMask::IsBitSet(jude_size_t index) const
   {
      return IsSet() && jude_bitfield_is_set(m_data, index);
   }

   void BitMask::ClearBit(jude_size_t index)
   {
      if (IsBitSet(index))
      {
         jude_bitfield_clear(m_data, index);
         jude_object_mark_field_changed(m_object.RawData(), m_fieldIndex, true);
      }
   }

   void BitMask::ClearAll()
   {
      if (IsSet())
      {
         jude_bitfield_clear_all(m_data, m_size);
         jude_object_mark_field_touched(m_object.RawData(), m_fieldIndex, false);
      }
   }

   void BitMask::SetBit(jude_size_t index)
   {
      if (!IsSet())
      {
         jude_bitfield_clear_all(m_data, m_size);
         jude_object_mark_field_touched(m_object.RawData(), m_fieldIndex, true);
      }

      if (!IsBitSet(index))
      {
         jude_bitfield_set(m_data, index);
         jude_object_mark_field_changed(m_object.RawData(), m_fieldIndex, true);
      }
   }

   bool BitMask::IsBitSet(const char* name) const
   {
      auto bit = jude_enum_find_value(&m_map, name);
      if (bit == nullptr)
      {
         return false;
      }
      return IsBitSet(*bit);
   }

   void BitMask::ClearBit(const char* name)
   {
      auto bit = jude_enum_find_value(&m_map, name);
      if (bit)
      {
         ClearBit(*bit);
      }
   }

   void BitMask::SetBit(const char* name)
   {
      auto bit = jude_enum_find_value(&m_map, name);
      if (bit)
      {
         SetBit(*bit);
      }
   }
}
