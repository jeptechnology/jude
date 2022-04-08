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
#include <vector>
#include <algorithm>

namespace jude
{
   namespace iteratordetail
   {
      template<typename, bool> struct AddConstIf;
      template<typename T> struct AddConstIf<T, true> { using type = typename std::add_const<T>::type; };
      template<typename T> struct AddConstIf<T, false> { using type = T; };
   }

   class BaseArray
   {
      bool Edit(bool edited);

   protected:
      Object         m_parent;
      jude_size_t    m_fieldIndex;

   public:
      jude_size_t    capacity() const;
      virtual jude_size_t count() const;
      bool           Full() const;
      bool           IsEmpty() const { return count() == 0; }
      virtual bool   RemoveAt(jude_size_t array_index);
      void           clear(bool withNotification = true);

      //  For the benefit of std library
      size_t         size() const { return count(); }

      //////////////////////////////////////////////////////////
      // Protobuf compatibility layer
      bool empty() const { return IsEmpty(); }
      void Clear() { clear(); }
      //////////////////////////////////////////////////////////

   protected:
      BaseArray(Object& object, jude_size_t fieldIndex);
      BaseArray(BaseArray& rhs) : m_parent(rhs.m_parent), m_fieldIndex(rhs.m_fieldIndex) {}

      bool Add(const void *value);
      bool Insert(jude_size_t array_index, const void *value);
      bool Set(jude_size_t array_index, const void *value);
      const void *Get(jude_size_t array_index) const;
      jude_size_t Read(void *destination, size_t max_elements) const;
      jude_size_t Write(const void *source, size_t max_elements);
   };

   template<class T>
   class Array : public BaseArray
   {
   public:
      Array(Object& object, jude_size_t fieldIndex)
         : BaseArray(object, fieldIndex)
      {}

      Array(Array& rhs) : BaseArray(rhs) {}

      const T operator[] (jude_size_t index) const { return Get(index); }
      bool Add(T value) { return BaseArray::Add(&value); }
      bool Insert(jude_size_t array_index, T value) { return BaseArray::Insert(array_index, &value); }
      bool Set(jude_size_t array_index, T value) { return BaseArray::Set(array_index, &value); }
      bool Contains(T value) const 
      { 
         for (const auto& v : *this) 
         {
            if (v == value) return true;
         }
         return false;
      }

      bool Erase(T value) 
      {
         for (int i = 0; i < count(); ++i)
         {
            if (Get(i) == value)
            {
               return RemoveAt(i);
            }
         }
         return false;
      }

      bool erase(T value) { return Erase(value); }

      const T Get(jude_size_t array_index) const { return *reinterpret_cast<const T*>(BaseArray::Get(array_index)); }

      jude_size_t Read(T* destination, size_t max_elements) const { return BaseArray::Read(destination, max_elements); }
      jude_size_t Write(const T* source, size_t max_elements) { return BaseArray::Write(source, max_elements); }
      jude_size_t Write(const std::vector<T>& vec) { return Write(vec.data(), vec.size()); }
      jude_size_t Write(std::initializer_list<T>& elements) { std::vector<T> vec(elements); return Write(vec); }

      //////////////////////////////////////////////////////////
      // Protobuf compatibility layer
      const T at(jude_size_t array_index) const { return Get(array_index); }
      jude_size_t set(const T* source, size_t max_elements) { return Write(source, max_elements); }
      auto push_back(T value) { return Add(value); }
      //////////////////////////////////////////////////////////

      const std::vector<T> AsVector() const
      {
         std::vector<T> v;
         for (T i : *this) v.push_back(i);
         return v;
      }

      class const_iterator
      {
         const Array<T>& m_array;
         mutable jude_size_t m_index{ 0 };
         jude_size_t m_last;

      public:
         using value_type = const T;
         using pointer = value_type *;
         using reference = value_type &;
         using iterator_category = std::forward_iterator_tag;
         using difference_type = std::ptrdiff_t;

         const_iterator(const const_iterator& rhs) : m_array(rhs.m_array), m_index(rhs.m_index), m_last(rhs.m_last) {}
         const_iterator& operator=(const const_iterator& rhs) { m_index = rhs.m_index; m_last = rhs.m_last; return *this; }

         const_iterator(const Array<T>& array, jude_size_t index) : m_array(array), m_index(index)
         {
            m_last = m_array.count();   // find last entry
            if (m_index > m_last)
            {
               m_index = m_last; // end() implementation uses this to mark end
            }
         }
         
         bool operator!=(const const_iterator& rhs) const { return m_index != rhs.m_index; }
         void operator++() const
         {
            if (++m_index > m_last)
            {
               m_index = m_last;
            }
         }
         const T operator* () { return m_array.Get(m_index); }
      };

      using value_type = T;
      using iterator = const_iterator; // we can't edit Array fields using the iterator!

      iterator begin() { return iterator(*this, 0); }
      iterator end() { return iterator(*this, (jude_size_t)-1); }
      const_iterator begin() const { return const_iterator(*this, (jude_size_t)0); }
      const_iterator end() const { return const_iterator(*this, (jude_size_t)-1); }
      const_iterator cbegin() const { return const_iterator(*this, (jude_size_t)0); }
      const_iterator cend() const { return const_iterator(*this, (jude_size_t)-1); }
   };
}
