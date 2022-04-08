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

#include <jude/core/cpp/PubSubInterface.h>
#include <functional>

namespace jude 
{
   struct ValidationResult 
   {
      bool isValid;
      std::string error;

      // NOTE: These constructors are encouraged to be used implcitly from a validation callback
      // examples:  
      //  return true;
      //  return false;    
      //  return "I got an error";
      ValidationResult(const char* failure)         : isValid(false),         error(failure ? failure : "(null)") {}
      ValidationResult(const std::string& failure)  : isValid(false),         error(failure)                {}
      ValidationResult(ValidationResult &&other)    : isValid(other.isValid), error(std::move(other.error)) {}
      ValidationResult(const ValidationResult &other) : isValid(other.isValid), error(other.error) {}
      ValidationResult(bool success)                : isValid(success),       error(success ? "" : "Generic Error") {}

      operator bool() const { return isValid; }
   };

   template<typename T_Object = Object>
   class Validation : public Notification<T_Object>
   {
      using EventSourceLocker = std::function<T_Object()>;
      
   public:
      // For new validation objects
      explicit Validation(const T_Object& object, EventSourceLocker sourceLocker, bool isDeleted = false)
         : Notification<T_Object>(object, sourceLocker, isDeleted)
      {}

      // NOTE: For translating
      explicit Validation(Object *object, EventSourceLocker sourceLocker, bool isDeleted)
         : Notification<T_Object>(object, sourceLocker, isDeleted)
      {}

      template<typename T>
      Validation<T> As(std::function<T()>&& sourceLocker)
      {
         static_assert(std::is_same<T_Object, Object>::value); // can only translate from Validation<Object>
         static_assert(std::is_base_of<Object, T>::value);     // can only translate to a dervied class of Object

         return Validation<T>(&this->m_copyOfChangedObject, sourceLocker, this->m_deleted);
      }

      // NOTE: Validators get write access to object not yet committed.
      // This allows "defaults" or "fix ups" to be applied automatically before the object hits the main database
      T_Object* operator->() { return &this->m_copyOfChangedObject; }
      T_Object& operator*()  { return this->m_copyOfChangedObject; }

   };

   template<class T_Object = Object>
   class Validatable
   {
   public:
      using Validator = std::function<ValidationResult(Validation<T_Object>&)>;

      // A validator is a type of subscriber that is run when we want to validate a resource
      virtual SubscriptionHandle ValidateWith(Validator callback) = 0;
   };
}
