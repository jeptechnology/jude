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

#include <jude/jude.h>
#include <jude/database/Transaction.h>
#include <jude/core/cpp/Object.h>

namespace jude
{
   class CollectionBase;
   
   /*
    * CollectionBaseIterator definition code to allow the ranged for-loop
    */
   struct CollectionBaseConstIterator
   {
      using iterator_category = std::input_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = const Object;
      using pointer           = const Object*;
      using reference         = const Object&;

      CollectionBaseConstIterator (const CollectionBaseConstIterator& rhs)
         : m_collection(rhs.m_collection)
         , m_object(*const_cast<Object*>(&rhs.m_object))
      {}

      CollectionBaseConstIterator(const CollectionBase& collection);
      CollectionBaseConstIterator(const CollectionBase& collection, jude_id_t id, bool findNext = false);

      CollectionBaseConstIterator& operator=(CollectionBaseConstIterator& rhs);
      CollectionBaseConstIterator& operator=(CollectionBaseConstIterator&& rhs);
      CollectionBaseConstIterator& operator++();
      CollectionBaseConstIterator  operator++(int);

      reference operator*() const { return m_object; }
      pointer operator->() const { return &m_object; }
      
      operator bool() const { return m_object.IsOK(); }
      bool IsOK() const { return m_object.IsOK(); }

      friend bool operator== (const CollectionBaseConstIterator& a, const CollectionBaseConstIterator& b) { return a.m_object == b.m_object; };
      friend bool operator!= (const CollectionBaseConstIterator& a, const CollectionBaseConstIterator& b) { return a.m_object != b.m_object; }; 

   protected:
      const CollectionBase* m_collection;
      mutable Object m_object;
   };

   struct CollectionBaseIterator
   {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = Object;
      using pointer           = Object*;
      using reference         = Object&;

      CollectionBaseIterator (const CollectionBaseIterator& rhs)
         : m_collection(rhs.m_collection)
         , m_object(*const_cast<pointer>(&rhs.m_object))
      {}

      CollectionBaseIterator& operator=(CollectionBaseIterator& rhs);
      CollectionBaseIterator& operator=(CollectionBaseIterator&& rhs);
      CollectionBaseIterator(CollectionBase& collection);
      CollectionBaseIterator(CollectionBase& collection, jude_id_t id, bool findNext = false);

      CollectionBaseIterator& operator++();
      CollectionBaseIterator operator++(int);

      reference operator*() { return m_object; }
      pointer operator->() { return &m_object; }
      
      operator bool () const { return m_object.IsOK(); }
      bool IsOK() const { return m_object.IsOK(); }

      friend bool operator== (const CollectionBaseIterator& a, const CollectionBaseIterator& b) { return a.m_object == b.m_object; };
      friend bool operator!= (const CollectionBaseIterator& a, const CollectionBaseIterator& b) { return a.m_object != b.m_object; }; 

   protected:
      CollectionBase* m_collection;
      mutable Object m_object;
   };

   template<class T_Object>
   struct CollectionConstIterator
   {
      using iterator_category = std::input_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = const T_Object;
      using pointer           = const T_Object*;
      using reference         = const T_Object&;

      CollectionConstIterator (const CollectionConstIterator& copy)
         : m_iter(copy.m_iter)
         , m_value(nullptr)
      {
         AssignTypedValue();
      }

      CollectionConstIterator(const CollectionBase& collection)
         : m_iter(collection)
         , m_value(nullptr)
      {}

      CollectionConstIterator(const CollectionBase& collection, jude_id_t id, bool findNext = false)
         : m_iter(collection, id, findNext)
         , m_value(nullptr)
      {
         AssignTypedValue();
      }

      CollectionConstIterator& operator=(CollectionConstIterator&& rhs)
      {
         m_iter = std::move(rhs.m_iter);
         AssignTypedValue();
         return *this;
      }

      CollectionConstIterator& operator=(CollectionConstIterator& rhs)
      {
         m_iter = rhs.m_iter;
         AssignTypedValue();
         return *this;
      }

      CollectionConstIterator& operator++()
      {
         m_iter++;
         AssignTypedValue();
         return *this;
      }

      CollectionConstIterator operator++(int)
      {
         CollectionConstIterator tmp = *this; 
         ++(*this); 
         return tmp; 
      }

      reference operator*() { return m_value; }
      pointer operator->() { return &m_value; }

      operator bool () const { return m_iter.IsOK(); }
      bool IsOK() const { return m_iter.IsOK(); }

      friend bool operator== (const CollectionConstIterator& a, const CollectionConstIterator& b) { return a.m_value == b.m_value; };
      friend bool operator!= (const CollectionConstIterator& a, const CollectionConstIterator& b) { return a.m_value != b.m_value; }; 

   private:
      void AssignTypedValue()
      {
         if (m_iter) 
         { 
            m_value = m_iter->CloneAs<T_Object>(); 
         }
         else
         {
            m_value = nullptr;
         }
      }

      CollectionBaseConstIterator m_iter;
      mutable T_Object m_value;
   };

   template<class T_Object>
   struct CollectionIterator
   {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = T_Object;
      using pointer           = T_Object*;
      using reference         = T_Object&;

      CollectionIterator(const CollectionIterator& copy)
         : m_iter(copy.m_iter)
         , m_value(nullptr)
      {
         AssignTypedValue();
      }

      CollectionIterator(CollectionBase& collection)
         : m_iter(collection)
         , m_value(nullptr)
      {}
      
      CollectionIterator(CollectionBase& collection, jude_id_t id, bool findNext = false)
         : m_iter(collection, id, findNext)
         , m_value(nullptr)
      {
         AssignTypedValue();
      }

      CollectionIterator& operator=(CollectionIterator&& rhs)
      {
         m_iter = std::move(rhs.m_iter);
         AssignTypedValue();
         return *this;
      }

      CollectionIterator& operator=(CollectionIterator& rhs)
      {
         m_iter = rhs.m_iter;
         AssignTypedValue();
         return *this;
      }

      CollectionIterator& operator++()
      {
         ++m_iter;
         AssignTypedValue();
         return *this;
      }

      CollectionIterator operator++(int)
      {
         CollectionIterator tmp = *this; 
         ++(*this); 
         return tmp; 
      }

      reference operator*() { return m_value; }
      pointer operator->() { return &m_value; }

      operator bool () const { return m_iter.IsOK(); }
      bool IsOK() const { return m_iter.IsOK(); }

      friend bool operator== (const CollectionIterator& a, const CollectionIterator& b) { return a.m_value == b.m_value; };
      friend bool operator!= (const CollectionIterator& a, const CollectionIterator& b) { return a.m_value != b.m_value; }; 
   
   private:
      void AssignTypedValue()
      {
         if (m_iter) 
         { 
            m_value = m_iter->As<T_Object>(); 
         }
         else
         {
            m_value = nullptr;
         }
      }

      CollectionBaseIterator m_iter;
      mutable T_Object m_value;
   };
}
