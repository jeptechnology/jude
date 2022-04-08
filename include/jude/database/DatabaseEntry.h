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

#include "../core/cpp/RestApiInterface.h"
#include <jude/core/cpp/PubSubInterface.h>
#include <jude/core/cpp/Validatable.h>
#include <set>
#include <memory>
#include <mutex>

#ifdef DELETE
#undef DELETE
#endif

namespace jude
{
   using PathNotifyCallback = std::function<void(const std::string& pathThatChanged, const Notification<Object>& info)>;

   // A DatabaseEntry has both a Rest API and can be subscribed to...
   class DatabaseEntry : public RestApiInterface
                       , public PubSubInterface<>
   {
   protected:

      friend class Database;

      mutable std::shared_ptr<jude::Mutex> m_mutex; // mutable as we may need to lock for read only operations too.

   public:
      explicit DatabaseEntry(std::shared_ptr<jude::Mutex> mutex)
         : m_mutex(mutex)
      {}

      enum class DBEntryType
      {
         DATABASE,
         COLLECTION,
         RESOURCE
      };

      virtual std::string GetName() const = 0;
      virtual jude_user_t GetAccessLevel(CRUD type) const = 0;
      virtual const jude_rtti_t* GetType() const = 0;
      virtual size_t SubscriberCount() const = 0;
      virtual void ClearAllDataAndSubscribers() = 0;
      virtual std::string DebugInfo() const = 0;
      
      virtual DBEntryType GetEntryType() const = 0;

      virtual SubscriptionHandle SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue) = 0;
      virtual bool Restore(std::string path, InputStreamInterface& input) = 0;

      // Functions to support Open Api Spec 3.0 generation
      virtual void OutputAllSchemasInYaml(OutputStreamInterface& output, std::set<const jude_rtti_t*>& alreadyDone, jude_user_t userLevel) const = 0;
      virtual void OutputAllSwaggerPaths(OutputStreamInterface& output, const std::string& prefix, jude_user_t userLevel) const = 0;
      virtual std::string GetSwaggerReadSchema(jude_user_t userLevel) const = 0;

      // Function outputs JSON-Schema
      virtual void OutputJsonSchema(OutputStreamInterface& output, jude_user_t userLevel) const
      {
         jude_create_default_json_schema(output.GetLowLevelOutputStream(), GetType(), userLevel);
      }

   };
}