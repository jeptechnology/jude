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

#include <jude/database/CollectionIterator.h>
#include <jude/database/Collection.h>

namespace jude
{
   CollectionBaseConstIterator::CollectionBaseConstIterator(const CollectionBase& collection)
      : m_collection(&collection)
      , m_object(nullptr)
   {}

   CollectionBaseConstIterator::CollectionBaseConstIterator(const CollectionBase& collection, jude_id_t id, bool findNext)
      : m_collection(&collection)
      , m_object(const_cast<CollectionBase*>(m_collection)->LockForEdit(id, findNext))
   {}

   CollectionBaseConstIterator& CollectionBaseConstIterator::operator=(CollectionBaseConstIterator& rhs)
   {
      m_collection = rhs.m_collection;
      m_object = rhs.m_object;
      return *this;
   }

   CollectionBaseConstIterator& CollectionBaseConstIterator::operator=(CollectionBaseConstIterator&& rhs)
   {
      m_collection = rhs.m_collection;
      m_object = std::move(rhs.m_object);
      return *this;
   }

   CollectionBaseConstIterator& CollectionBaseConstIterator::operator++()
   {
      if (m_object)
      {
         m_object = const_cast<CollectionBase*>(m_collection)->LockForEdit(m_object.Id(), true);
      }
      return *this;
   }

   CollectionBaseConstIterator CollectionBaseConstIterator::operator++(int) 
   { 
      CollectionBaseConstIterator tmp = *this; 
      ++(*this); 
      return tmp; 
   }


   CollectionBaseIterator::CollectionBaseIterator(CollectionBase& collection)
      : m_collection(&collection)
      , m_object(nullptr)
   {
   }

   CollectionBaseIterator::CollectionBaseIterator(CollectionBase& collection, jude_id_t id, bool findNext)
      : m_collection(&collection)
      , m_object(m_collection->LockForEdit(id, findNext))
   {}

   CollectionBaseIterator& CollectionBaseIterator::operator=(CollectionBaseIterator& rhs)
   {
      m_collection = rhs.m_collection;
      m_object = rhs.m_object;
      return *this;
   }

   CollectionBaseIterator& CollectionBaseIterator::operator=(CollectionBaseIterator&& rhs)
   {
      m_collection = rhs.m_collection;
      m_object = std::move(rhs.m_object);
      return *this;
   }

   CollectionBaseIterator& CollectionBaseIterator::operator++()
   {
      if (m_object)
      {
         if(auto next = m_collection->GenericLockNext(m_object.Id()))
         {
            m_object = *next;
         }
         else
         {
            m_object = nullptr;
         }

      }
      return *this;
   }

   CollectionBaseIterator CollectionBaseIterator::operator++(int) 
   { 
      CollectionBaseIterator tmp = *this; 
      ++(*this); 
      return tmp; 
   }
}
