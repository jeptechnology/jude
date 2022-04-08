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

#include <jude/core/cpp/ObjectArray.h>

namespace jude
{
   GenericObjectArray::GenericObjectArray(Object& parent, jude_size_t fieldIndex)
      : BaseArray(parent, fieldIndex)
   {
   }

   jude_size_t GenericObjectArray::count() const
   {
      return jude_object_count_subresources(m_parent.RawData(), m_fieldIndex);
   }
   
   bool GenericObjectArray::RemoveId(jude_id_t id)
   {
      return jude_object_remove_subresource(m_parent.RawData(), m_fieldIndex, id);
   }
   
   Object GenericObjectArray::_Add(jude_id_t requested_id)
   {
      if (auto subresource = jude_object_add_subresource(m_parent.RawData(), m_fieldIndex, requested_id))
      {
         return Object(m_parent, *subresource);
      }
      return nullptr;
   }
   
   Object GenericObjectArray::_Find(jude_id_t id)
   {
      if (auto subresource = jude_object_find_subresource(m_parent.RawData(), m_fieldIndex, id))
      {
         return Object(m_parent, *subresource);
      }
      return nullptr;
   }

   const Object GenericObjectArray::_Find(jude_id_t id) const
   {
      return const_cast<GenericObjectArray*>(this)->_Find(id);
   }

   Object GenericObjectArray::_At(jude_size_t array_index)
   {
      if (auto subresource = (jude_object_t *)jude_object_get_subresource_at_index(m_parent.RawData(), m_fieldIndex, array_index))
      {
         return Object(m_parent, *subresource);
      }
      return nullptr;
   }

   const Object GenericObjectArray::_At(jude_size_t array_index) const
   {
      return const_cast<GenericObjectArray*>(this)->_At(array_index);
   }

}
