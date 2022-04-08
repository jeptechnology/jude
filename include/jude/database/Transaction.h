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

#include <jude/core/cpp/Object.h>
#include <jude/porting/Mutex.h>
#include <mutex>
#include <memory>
#include <functional>

namespace jude
{
   class Database;

   template<class T_Object>
   class Transaction
   {
   public:
      using TransactionCompleteFn = std::function<RestfulResult(Object& resource, bool commit)>;

   private:
      std::unique_lock<jude::Mutex> m_lock;
      bool                  m_needsCommit{true};
      TransactionCompleteFn m_onTransactionComplete;
      std::string           m_error;
      T_Object              m_object;

      void CommitOnDestructor()
      {
         auto result = Commit();
         if (!result)
         {
            #if defined(JUDE_TRANSACTION_DESTRUCTOR_COMMIT_FAILURE_IS_FATAL)
               auto message = "ERROR: Transaction failed in destructor: " + result.GetDetails();
               jude_fatal(message.c_str());
            #else
               // Failure in a destructor... we must at least warn!!
               jude_debug("ERROR: Transaction failed with '%s'", result.GetDetails().c_str());
            #endif
         }
      }

   public:
      Transaction(std::string m_error)
         : m_object(nullptr)
         , m_error(m_error)
      {}

      Transaction(std::nullptr_t)
         : Transaction("(null)")
      {}

      Transaction(std::unique_lock<jude::Mutex>&& lock, const Object& object, TransactionCompleteFn onTransactionComplete)
         : m_lock(std::move(lock))
         , m_object(object.Clone(false).template As<T_Object>())
         , m_onTransactionComplete(onTransactionComplete)
      {} 

      Transaction(Transaction&& rhs)
         : m_lock(std::move(rhs.m_lock))
         , m_object(std::move(rhs.m_object))
         , m_error(std::move(rhs.m_error))
         , m_onTransactionComplete(std::move(rhs.m_onTransactionComplete))
      {}

      ~Transaction()
      {
         CommitOnDestructor();
      }

      // Transactions on searches may not be able to do anything useful
      bool IsOK() const { return m_object.IsOK(); }
      void AssertOK() const { jude_assert(IsOK()); }
      operator bool() const { return IsOK(); }
      const std::string& GetError() { return m_error; }
      const char * GetErrorMessage() { return m_error.c_str(); }

      
      T_Object* get() { return &m_object; } // do your own assertion
      const T_Object* get() const { return &m_object; } // do your own assertion

      T_Object&       operator *()       { AssertOK(); return m_object; }
      const T_Object& operator *() const { AssertOK(); return m_object; }

      T_Object*       operator ->()       { AssertOK(); return &m_object; }
      const T_Object* operator ->() const { AssertOK(); return &m_object; }

      RestfulResult Commit()
      {
         if (m_object && m_onTransactionComplete)
         {
            auto result = m_onTransactionComplete(m_object, m_needsCommit);
            m_needsCommit = false; // prevent double commit
            m_object = nullptr; // destroy the object
            return result;
         }
         return jude_rest_OK;
      }

      void Abort()
      {
         // we can't commit if we remove the commit function
         m_needsCommit = false;
      }
   };
}
