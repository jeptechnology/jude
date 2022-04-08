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
#include <jude/core/cpp/FieldMask.h>
#include <jude/database/Transaction.h>

#include <string>
#include <vector>
#include <functional>


namespace jude
{
   template<typename T_Object = Object>
   class Notification
   {
   public:
      using EventSourceLocker = std::function<T_Object()>; // function that locks the original source for this notification event

      explicit Notification(const Object& object, EventSourceLocker sourceLocker, bool isDeleted = false) 
         : m_deleted(isDeleted)
         , m_copyOfChangedObject(object.template CloneAs<T_Object>(false)) // clone but don't clear any change markers!
         , m_sourceLocker(sourceLocker)
         , updatedFields(object.GetChanges())
         , messageAccessor(m_copyOfChangedObject)
      {}

      Notification(const Notification& rhs)
         : m_deleted(rhs.m_deleted)
         , m_copyOfChangedObject((Object&)rhs.m_copyOfChangedObject)
         , m_sourceLocker(rhs.m_sourceLocker)
         , updatedFields(rhs.updatedFields)
         , messageAccessor(m_copyOfChangedObject)
      {}

      explicit Notification(Object* alreadyCopiedObject, EventSourceLocker sourceLocker, bool isDeleted = false) 
         : m_deleted(isDeleted)
         , m_copyOfChangedObject(alreadyCopiedObject->As<T_Object>()) // same reference 
         , m_sourceLocker(sourceLocker)
         , updatedFields(m_copyOfChangedObject.GetChanges())
         , messageAccessor(m_copyOfChangedObject)
      {}

      // translate notification
      template<typename T>
      Notification<T> As(std::function<T()>&& sourceLocker) const
      {
         static_assert(std::is_same<T_Object, Object>::value); // can only translate from Notification<Object>
         static_assert(std::is_base_of<Object, T>::value);     // can only translate to a dervied class of Object

         return Notification<T>(m_copyOfChangedObject, [&] { return this->Source().template As<T>(); }, m_deleted);
      }

      bool IsDeleted() const { return m_deleted; }
      bool IsNew() const { return !m_deleted && m_copyOfChangedObject.IsChanged(JUDE_ID_FIELD_INDEX); }

      const T_Object* operator->() const { return &m_copyOfChangedObject; }
      const T_Object& operator*() const { return m_copyOfChangedObject; }

      // Locks the event source object in the database that caused this notification - NOTE: If deleted, this may return empty object
      T_Object Source() const { return m_sourceLocker ? m_sourceLocker() : T_Object(nullptr); }
      
      FieldMask GetChangeMask() const 
      { 
         if (IsDeleted())
         {
            return FieldMask::ForFields({JUDE_ID_FIELD_INDEX});
         }
         return m_copyOfChangedObject.GetChanges();
      }

      operator bool() const { return !IsDeleted(); }

   protected:
      bool m_deleted;
      T_Object m_copyOfChangedObject; // copy of the changed object - even a deleted object will have a copy of its last state.
      EventSourceLocker m_sourceLocker;

   public:
      //////////////////////////////////////////////////////////////////////////////
      // Protobuf compatibility layer
      const FieldMask updatedFields;
      const T_Object& messageAccessor;
      const T_Object& GetMessageAccessor() const { return messageAccessor; }
      //////////////////////////////////////////////////////////////////////////////
   };
}

