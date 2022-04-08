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

#include <jude/core/cpp/AtomicArray.h>

namespace jude
{

   BaseArray::BaseArray(Object& parent, jude_size_t fieldIndex)
      : m_parent(parent)
      , m_fieldIndex(fieldIndex)
   {}

   bool BaseArray::Edit(bool edited)
   {
      if (edited)
      {
         m_parent.OnEdited();
      }
      return edited;
   }


   jude_size_t BaseArray::capacity() const
   {
      return m_parent.Type().field_list[m_fieldIndex].array_size;
   }

   bool BaseArray::Full() const
   {
      return count() == capacity();
   }

   jude_size_t BaseArray::count() const
   {
      return jude_object_count_field(m_parent.RawData(), m_fieldIndex);
   }
   
   void BaseArray::clear(bool withNotification)
   {
      jude_object_clear_array(m_parent.RawData(), m_fieldIndex);
      if (withNotification)
      {
         m_parent.OnEdited();
      }
   }

   bool BaseArray::RemoveAt(jude_size_t array_index)
   {
      return Edit(jude_object_remove_value_from_array(m_parent.RawData(), m_fieldIndex, array_index));
   }

   bool BaseArray::Add(const void *value)
   {
      return Edit(jude_object_insert_value_into_array(m_parent.RawData(), m_fieldIndex, count(), value));
   }

   bool BaseArray::Insert(jude_size_t array_index, const void *value)
   {
      return Edit(jude_object_insert_value_into_array(m_parent.RawData(), m_fieldIndex, array_index, value));
   }

   bool BaseArray::Set(jude_size_t array_index, const void *value)
   {
      return Edit(jude_object_set_value_in_array(m_parent.RawData(), m_fieldIndex, array_index, value));
   }

   const void *BaseArray::Get(jude_size_t array_index) const
   {
      return jude_object_get_value_in_array(m_parent.RawData(), m_fieldIndex, array_index);
   }

   jude_size_t BaseArray::Read(void *destination, size_t max_elements) const
   {
      return jude_object_copy_from_array(m_parent.RawData(), m_fieldIndex, destination, (jude_size_t)max_elements);
   }

   jude_size_t BaseArray::Write(const void *source, size_t max_elements)
   {
      clear(false); // clear without notification
      auto elementSize = m_parent.Type().field_list[m_fieldIndex].data_size;
      jude_size_t index = 0;
      for (index = 0; index < max_elements; index++)
      {
         if (!jude_object_insert_value_into_array(m_parent.RawData(), m_fieldIndex, index, (char*)source + (index * elementSize)))
         {
            break;
         }
      }
      Edit(true);
      return index; // number of elements copied
   }
}
