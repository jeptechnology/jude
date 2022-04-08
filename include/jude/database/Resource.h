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

#include <mutex>
#include <memory>
#include <map>
#include <jude/jude.h>
#include <jude/core/cpp/Validatable.h>
#include <jude/database/DatabaseEntry.h>
#include <jude/database/Transaction.h>

namespace jude
{
   class GenericResource : public DatabaseEntry
                         , public Validatable<>
   {
      uint32_t nextSubscriberId {0};
      void OnChanged();

   protected:
      std::string    m_name;
      struct {
         jude_user_t canRead;
         jude_user_t canUpdate;
      } m_access;
      
      Object         m_object;
      bool           m_notifyImmediatelyOnChange{false}; // normally we wait until the full edit is completed

      struct IndividualSubscriber 
      {
         FieldMask filter;
         Subscriber callback;
         NotifyQueue* queue;         
      };
      std::map<uint32_t, IndividualSubscriber> m_subscribers;
      std::map<uint32_t, Validatable<>::Validator> m_validators;

      explicit GenericResource(const std::string& name, const jude_rtti_t& type, jude_user_t accessLevel, std::shared_ptr<jude::Mutex> mutex);

      RestfulResult PatchObject(Object& newObject);
      RestfulResult PutObject(Object& newObject);

      SubscriptionHandle OnChangeToPath(
         const std::string& subscriptionPath,
         Subscriber callback,
         FieldMask resourceFieldFilter,
         NotifyQueue& queue) override;

      void Unsubscribe(uint32_t subscriberId);
      void DeregisterValidator(uint32_t validateorId);

      void PublishChangesToQueue();
      void HandleChangesFromQueue(Notification<Object> notification, NotifyQueue* origin);

      void OnEditCompleted();
      RestfulResult OnTransactionCompleted(Object& editedCopy, bool needsCommit);
      ValidationResult Validate(Validation<Object>& info);

   public:
      std::string GetName() const { return m_name; }
      const jude_rtti_t* GetType() const override { return &m_object.Type(); }
      jude_user_t GetAccessLevel(CRUD crud) const override;
      void        SetAccessLevel(CRUD crud, jude_user_t);
      virtual void ClearAllDataAndSubscribers() override;
      virtual size_t SubscriberCount() const override { return m_subscribers.size(); }

      // From RestApiInterface...
      virtual RestfulResult RestGet(const char* path, OutputStreamInterface& output, const AccessControl& accessControl = accessToEverything) const override;
      virtual RestfulResult RestPost(const char* path, InputStreamInterface& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestPatch(const char* path, InputStreamInterface& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestPut(const char* path, InputStreamInterface& input, const AccessControl& accessControl = accessToEverything) override;
      virtual RestfulResult RestDelete(const char* path, const AccessControl& accessControl = accessToEverything) override;

      virtual std::vector<std::string> SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel = jude_user_Root) const override;

      virtual std::string DebugInfo() const override;
      virtual void OutputAllSchemasInYaml(jude::OutputStreamInterface& output, std::set<const jude_rtti_t*>& alreadyDone, jude_user_t userLevel) const override;
      virtual void OutputAllSwaggerPaths(jude::OutputStreamInterface& output, const std::string& prefix, jude_user_t userLevel) const override;
      virtual std::string GetSwaggerReadSchema(jude_user_t userLevel) const override;

      virtual DBEntryType GetEntryType() const override { return DBEntryType::RESOURCE; }

      SubscriptionHandle ValidateWith(Validatable<>::Validator callback) override;
      SubscriptionHandle SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue) override;
      bool Restore(std::string path, InputStreamInterface& input) override;

      Object              GenericLock();
      Transaction<Object> GenericTransaction();
   };

   template<class T_Object>
   class Resource : public GenericResource
                  , public PubSubInterface<T_Object>
                  , public Validatable<T_Object>
   {
      static_assert(std::is_base_of<Object, T_Object>::value, "T_Object must derive from jude::Object");

      using T_Notification = Notification<T_Object>;
      using T_Subscriber   = typename PubSubInterface<T_Object>::T_Subscriber;
      using T_Validator    = typename Validatable<T_Object>::Validator;

      Resource(const Resource&) = delete;

   public:
      explicit Resource(const char* name, jude_user_t accessLevel = jude_user_Public, std::shared_ptr<jude::Mutex> mutex = std::make_shared<jude::Mutex>())
         : GenericResource(name, *T_Object::RTTI(), accessLevel, mutex)
      {}

      SubscriptionHandle OnChange(T_Subscriber callback,
                             FieldMask resourceFieldFilter = FieldMask::ForAllChanges(),
                             NotifyQueue& queue = NotifyQueue::Default) override
      {
         return OnChangeToPath(
            "", 
            [=](const Notification<Object>& genericNotification) 
            {
               callback(genericNotification.As<T_Object>([&] { return WriteLock(); }));
            }, 
            resourceFieldFilter, 
            queue);
      }

      SubscriptionHandle ValidateWith(T_Validator callback) override
      {
         return GenericResource::ValidateWith([=](Validation<Object>& info)
            {
               auto id = info->Id();
               auto typedInfo = info.As<T_Object>([&, id]() { return WriteLock(); });
               return callback(typedInfo);
            });
      }

      // No copy, locks the resource until the transaction completes
      T_Object WriteLock()
      {
         return GenericLock().As<T_Object>();
      }

      // Makes copy, locks the resource until the transaction completes
      Transaction<T_Object> TransactionLock()
      {
         std::unique_lock<jude::Mutex> lock(*m_mutex);

         return Transaction<T_Object>(
            std::move(lock),
            m_object,
            [&](Object& resource, bool needsCommit) ->RestfulResult
            {
               return OnTransactionCompleted(resource, needsCommit);
            });
      }

      auto operator ->()  { return std::make_unique<T_Object>(WriteLock()); }
   };
}