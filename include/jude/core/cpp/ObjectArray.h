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

#include <initializer_list>
#include <string>

#include <jude/jude_core.h>
#include <jude/core/cpp/AtomicArray.h>
#include <jude/core/cpp/Notification.h>
#include <jude/core/cpp/PubSubInterface.h>

namespace jude
{
   class GenericObjectArray : public BaseArray
   {
   public:
      GenericObjectArray(Object& parent, jude_size_t fieldIndex);
      GenericObjectArray(GenericObjectArray& rhs) : BaseArray(rhs) {}

      jude_size_t    count() const override;

      // Note: This removes using "id" search
      virtual bool   RemoveId(jude_id_t id);

   protected:
      Object _Add(jude_id_t requested_id = JUDE_AUTO_ID);
      
      // search using "id"
      Object _Find(jude_id_t id);
      const Object _Find(jude_id_t id) const;
      
      // get using "index"
      Object _At(jude_size_t index);
      const Object _At(jude_size_t index) const;
   };

   template<class T_Object>
   class ObjectArray : public GenericObjectArray
   {
   public:

      template<bool>class Iterator;
      using iterator = Iterator<false>;
      using const_iterator = Iterator<true>;
      using value_type = T_Object;

      ObjectArray(Object& parent, jude_size_t fieldIndex)
         : GenericObjectArray(parent, fieldIndex)
      {}

      ObjectArray(ObjectArray& rhs): GenericObjectArray(rhs) {}

      /*
       * Type-safe Accessors
       */
      std::optional<T_Object> Add(jude_id_t requested_id = JUDE_AUTO_ID)
      {
         if (auto object = _Add(requested_id))
         {
            return object.As<T_Object>();
         }
         return {};
      }

      std::optional<T_Object> Find(jude_id_t id)
      {
         if (auto object = _Find(id))
         {
            return object.As<T_Object>();
         }
         return {};
      }

      std::optional<const T_Object> Find(jude_id_t id) const
      {
         if (auto object = _Find(id))
         {
            return object.template CloneAs<T_Object>();
         }
         return {};
      }

      std::optional<T_Object> Find_If(std::function<bool(const T_Object&)> pred)
      {
         for (auto& object : *this)
         {
            if (pred(object))
            {
               return object;
            }
         }
         return {};
      }

      std::optional<const T_Object> Find_If(std::function<bool(const T_Object&)> pred) const
      {
         for (const auto& object : *this)
         {
            if (pred(object))
            {
               return object.Clone();
            }
         }
         return {};
      }

      T_Object operator[](jude_size_t id)
      {
         return _At(id).As<T_Object>();
      }

      const T_Object operator[](jude_size_t id) const
      {
         return const_cast<ObjectArray*>(this)->operator[](id);
      }

      const_iterator begin() const { return const_iterator(*const_cast<ObjectArray<T_Object>*>(this), 0); }
      const_iterator end() const   { return const_iterator(*const_cast<ObjectArray<T_Object>*>(this), (jude_size_t)-1); }
      iterator begin()  { return iterator(*this, 0); }
      iterator end()  { return iterator(*this, (jude_size_t)-1); }

      /*
      * Iterator class
      */
      template<bool IsConst>
      class Iterator
      {
         ObjectArray<T_Object>* m_array;
         mutable jude_size_t m_index{ 0 };
         mutable T_Object m_current;
         jude_size_t m_last;

      protected:
         T_Object GetObject() const { return m_array->_At(m_index).As<T_Object>(); }

      public:
         using value_type = typename iteratordetail::AddConstIf<T_Object, IsConst>::type;
         using pointer = value_type *;
         using reference = value_type &;
         using iterator_category = std::forward_iterator_tag;
         using difference_type = std::ptrdiff_t;

         Iterator(ObjectArray<T_Object>& array, jude_size_t index) : m_array(&array), m_index(index)
         {
            for (m_last = jude_object_count_field(m_array->m_parent.RawData(), m_array->m_fieldIndex);   // find last non-deleted entry
                 m_last > 0 && !m_array->_At(m_last - 1);
                 m_last--);

            if (m_index > m_last)
            {
               m_index = m_last; // end() implementation uses this to mark end
            }
            else
            {
               // find first valid entry after given index
               while (m_index < m_last && !m_array->_At(m_index))
               {
                  m_index++;
               }
            }
            m_current = GetObject();
         }

         Iterator &operator=(const Iterator& other)
         {
            m_array = other.m_array;
            m_index = other.m_index;
            m_last  = other.m_last;
            return *this;
         }

         bool operator!=(const Iterator& rhs) const { return m_index != rhs.m_index; }
         bool operator==(const Iterator& rhs) const { return m_index == rhs.m_index; }
         void operator++() const
         {
            while (++m_index < m_last 
                 && !m_array->_At(m_index)); // skip to next non-deleted
            m_current = GetObject();
         }
         operator bool() const { return m_array->_At(m_index).IsOK(); }

         reference operator* () const { return m_current; }
         pointer   operator-> () const { return &m_current; }
      };
   };
}
