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

#include <map>
#include <set>
#include <mutex>
#include <functional>
#include <algorithm>

#include <jude/jude.h>
#include <jude/core/cpp/Validatable.h>
#include "DatabaseEntry.h"
#include "Transaction.h"
#include "CollectionIterator.h"

namespace jude
{
   class CollectionBase : public DatabaseEntry
                        , public Validatable<>
   {
      static constexpr jude_id_t AllObjects = -1;

      friend struct CollectionBaseIterator;
      friend struct CollectionBaseConstIterator;

      uint32_t nextSubscriberId = 0;

      const jude_rtti_t            &m_rtti; // type of resources stored in this CollectionBase
      std::string                   m_name;
      struct {
         jude_user_t canCreate;
         jude_user_t canRead;
         jude_user_t canUpdate;
         jude_user_t canDelete;
      } m_access;
      
      std::map<jude_id_t, Object> m_objects;
      const size_t                m_capacity;
      
      struct CollectionSubscriber
      {
         FieldMask    filter;
         Subscriber   callback;
         NotifyQueue* queue;
         jude_id_t    id;   
         size_t       ptrdiff;      
      };
      std::unordered_map<uint32_t, CollectionSubscriber> m_subscribers;
      std::unordered_map<uint32_t, Validatable<>::Validator> m_validators;

      ValidationResult Validate(Validation<Object>& resource);
      void OnChanged();

      void PublishChangesToQueue(jude_id_t id);
      void PublishChangesToQueue(Object& changedObject, bool isDeleted);
      void HandleChangesFromQueue(const Notification<Object>& notification, NotifyQueue* origin);
      jude_id_t FindObjectIdFromPath(const char* path_token) const;

   protected:
      CollectionBase(const CollectionBase&) = delete;

      explicit CollectionBase(const jude_rtti_t &RTTI, const std::string& name, jude_user_t accessLevel, size_t capacity, std::shared_ptr<jude::Mutex> mutex);
      virtual ~CollectionBase() {}

      RestfulResult Post(const Object& newObject, bool generate_uuid, bool andValidate); // create new (new uuid is generated unless specified)

      // Edit locks - these won't validate but will lock for editing by code.
      Object LockForEdit(jude_id_t id, bool next = false);
      Object LockForEditFromPath(const char** fullpath, bool& isRootPath);
      void   OnEdited(jude_id_t id); // We can configure the collection to call this each time a change is made
      void   OnEditCompleted(jude_id_t id);

      // Transactions - these will validate before commiting
      template<class T_Object = Object>
      Transaction<T_Object> LockForTransaction(jude_id_t id)
      {
         // Lock now - pass this into the transaction to keep locked until the transaction completes.
         std::unique_lock<jude::Mutex> lock(*m_mutex);

         auto objectRecord = m_objects.find(id);
         if (objectRecord == m_objects.end())
         {
            return nullptr;
         }
         
         return Transaction<T_Object>(
            std::move(lock),
            objectRecord->second,    // we make a copy for transactions
            [this, id](Object& resource, bool needsCommit)->RestfulResult
            {
               return OnTransactionCompleted(id, resource, needsCommit);
            }
         );
      }

      Transaction<Object> CreateTransactionFromPath(const char** fullpath, bool& isRootPath);
      RestfulResult       OnTransactionCompleted(jude_id_t id, Object& copy, bool needsCommit);


      void Unsubscribe(uint32_t subscriberId);
      void DeregisterValidator(uint32_t validatorId);

      template<class T_Object = Object>
      Transaction<T_Object> CreatePostTransaction(jude_id_t id = JUDE_AUTO_ID, bool andValidate = true)
      {
         if (Full())
         {
            return nullptr;
         }

         if (id == JUDE_AUTO_ID)
         {
            id = jude_generate_uuid();
         }
         
         // Lock now - pass this into the transaction to keep locked until the transaction completes.
         std::unique_lock<jude::Mutex> lock(*m_mutex); 

         Object newObject(m_rtti);

         newObject.AssignId(id);

         return Transaction<T_Object>(
            std::move(lock),
            newObject,
            [this, id, andValidate](Object& resource, bool needsCommit)->RestfulResult
            {
               if (needsCommit)
               {
                  resource.AssignId(id); // NOTE: You can't change id once post has started
                  return Post(resource, false, andValidate);
               }
               return jude_rest_OK;
            }
         );
      }

      void HandleObjectNotification(const jude_notification_t* notify_data);
      void ProcessNotification(const jude_notification_t* notify_data);

   public:
      std::string GetName() const { return m_name; }
      const jude_rtti_t* GetType() const override { return &m_rtti; }
      jude_user_t GetAccessLevel(CRUD crud) const override;
      void        SetAccessLevel(CRUD crud, jude_user_t);

      using value_type     = jude::Object;
      using iterator       = CollectionBaseIterator;
      using const_iterator = CollectionBaseConstIterator;

      iterator       begin()        { return iterator(*this, JUDE_INVALID_ID, true); }
      iterator       end()          { return iterator(*this); }
      const_iterator begin() const  { return const_iterator(*this, JUDE_INVALID_ID, true); }
      const_iterator end() const    { return const_iterator(*this); }
      const_iterator cbegin() const { return const_iterator(*this, JUDE_INVALID_ID, true); }
      const_iterator cend() const   { return const_iterator(*this); }

      const_iterator GenericLock(jude_id_t id) const     { return const_iterator(*this, id); }
      const_iterator GenericLockNext(jude_id_t id) const { return const_iterator(*this, id, true); }
      const_iterator operator()(jude_id_t id) const      { return const_iterator(*this, id); }
      iterator       GenericLock(jude_id_t id)           { return iterator(*this, id); }
      iterator       GenericLockNext(jude_id_t id)       { return iterator(*this, id, true); }
      iterator       operator[](jude_id_t id)            { return iterator(*this, id); }

      virtual void ClearAllDataAndSubscribers() override;
      virtual size_t SubscriberCount() const override { return m_subscribers.size(); }

      RestfulResult RestoreEntry(InputStreamInterface& input); // for restoring from persistence
      RestfulResult Delete(jude_id_t id);
      void clear();
      size_t count() const;
      size_t size() const { return count(); }
      size_t capacity() const { return m_capacity; }
      bool Full() const { return count() >= capacity(); }

      bool ContainsId(jude_id_t id) const;
      std::vector<jude_id_t> GetIds() const;

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
      virtual std::string GetSwaggerReadSchema(jude_user_t userLevel) const;

      virtual DBEntryType GetEntryType() const override { return DBEntryType::COLLECTION; }

      SubscriptionHandle ValidateWith(Validatable<>::Validator callback) override;

      // Make a lock on the CollectionBase for a new entry - entry will only be added if transaction completes successfully
      Transaction<Object> GenericPostLock(jude_id_t id = JUDE_AUTO_ID);

      virtual SubscriptionHandle SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue) override;

      // From PubSubInterface<>
      virtual SubscriptionHandle OnChangeToPath(
         const std::string& subscriptionPath,
         Subscriber callback,
         FieldMask resourceFieldFilter = FieldMask::ForAllChanges(),
         NotifyQueue& queue = NotifyQueue::Default) override;

      bool Restore(std::string path, InputStreamInterface& input) override;

      RestfulResult PostObject(const Object&);  // ignores id
      RestfulResult PatchObject(const Object&); // needs id
   };

   template<class T_Object>
   class Collection : public CollectionBase
                    , public PubSubInterface<T_Object>
                    , public Validatable<T_Object>
   {
      static_assert(std::is_base_of<Object, T_Object>::value, "T_Object must derive from jude::Object");

      static constexpr size_t DefaultCapacity = 1024;

      using T_Notification = Notification<T_Object>;
      using T_Subscriber   = typename PubSubInterface<T_Object>::T_Subscriber;
      using T_Validator    = typename Validatable<T_Object>::Validator;
      using T_SourceLocker = typename T_Notification::EventSourceLocker;

      Collection(const Collection&) = delete;

   public:
      Collection(const char* name, size_t capacity = DefaultCapacity, jude_user_t accessLevel = jude_user_Public, std::shared_ptr<jude::Mutex> mutex = std::make_shared<jude::Mutex>())
         : CollectionBase(*T_Object::RTTI(), name, accessLevel, capacity, mutex)
      {}

      using value_type = T_Object;
      using iterator = CollectionIterator<T_Object>;
      using const_iterator = CollectionConstIterator<T_Object>;

      iterator begin()              { return iterator(*this, JUDE_INVALID_ID, true); }
      iterator end()                { return iterator(*this); }
      const_iterator begin() const  { return const_iterator(*this, JUDE_INVALID_ID, true); }
      const_iterator end() const    { return const_iterator(*this); }
      const_iterator cbegin() const { return const_iterator(*this, JUDE_INVALID_ID, true); }
      const_iterator cend() const   { return const_iterator(*this); }

      template<typename Pred>
      const_iterator find_if(Pred p) const { return std::find_if(cbegin(), cend(), p); }

      template<typename Pred>
      iterator find_if(Pred p) { return std::find_if(begin(), end(), p); }

      template<typename Pred>
      void remove_if(Pred p) { if (auto it = std::find_if(begin(), end(), p)) Delete(it->Id()); }

      iterator WriteLock(jude_id_t id)       {  return iterator(*this, id); }
      iterator WriteLockNext(jude_id_t id)   {  return iterator(*this, id, true); }
      iterator operator[](jude_id_t id)      {  return iterator(*this, id); }

      const_iterator ReadLock(jude_id_t id) const      {  return const_iterator(*this, id); }
      const_iterator ReadLockNext(jude_id_t id) const  {  return const_iterator(*this, id, true); }
      const_iterator operator()(jude_id_t id) const    {  return const_iterator(*this, id); }

      Transaction<T_Object> Post(jude_id_t id = JUDE_AUTO_ID)   {  return CreatePostTransaction<T_Object>(id, !Options::ValidatePostOnlyForRestAPI);  }
    
      Transaction<T_Object> TransactionLock(jude_id_t id)   {  return LockForTransaction<T_Object>(id);  }

      SubscriptionHandle OnChange(T_Subscriber callback,
                            FieldMask resourceFieldFilter = FieldMask::ForAllChanges(),
                            NotifyQueue& queue = NotifyQueue::Default) override
      {
         return OnChangeToPath(
            "",
            [=](const Notification<Object>& genericNotification)
            {
               auto id = genericNotification->Id();
               T_SourceLocker lockFunction = [&, id]() { return *WriteLock(id); };
               if (!genericNotification)
               {
                  lockFunction = {}; // make empty function if resource is not available to lock
               }
               callback(genericNotification.As<T_Object>(std::move(lockFunction)));
            },
            resourceFieldFilter,
            queue);
      }

      SubscriptionHandle ValidateWith(T_Validator callback) override
      {
         return CollectionBase::ValidateWith([=] (Validation<Object>& info)
            {
               auto id = info->Id();
               auto typedInfo = info.As<T_Object>([&, id]() { return *WriteLock(id); });
               return callback(typedInfo);
            });
      }

      template<typename Pred>
      std::vector<T_Object> find_all(Pred p) 
      {
         std::vector<T_Object> results;
         std::for_each(begin(), end(), [&] (T_Object& element) {
            if (p(element))
            {
               results.emplace_back(element);
            }
         });  
         return results;
      }

      std::vector<T_Object> AsVector() 
      {
         return find_all([] (auto&) { return true; });
      }

   };
}
