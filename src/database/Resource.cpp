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

#include <jude/database/Resource.h>
#include <jude/database/Swagger.h>
#include <algorithm>
#include <mutex>
#include <set>
#include <sstream>

using namespace std;

namespace jude
{
   GenericResource::GenericResource(const string& name, const jude_rtti_t& type, RestApiSecurityLevel::Value accessLevel, std::shared_ptr<jude::Mutex> mutex)
      : DatabaseEntry(mutex)
      , m_name(name)
      , m_object(type, 
                 [&] { OnChanged(); },
                 [&] { OnEditCompleted(); })
   {
      m_access.canRead = accessLevel;
      m_access.canUpdate = accessLevel;

      m_object.AssignId(jude_generate_uuid());
      m_object.ClearChangeMarkers();
   }

   void GenericResource::OnChanged()
   {
      PublishChangesToQueue();
   }

   void GenericResource::OnEditCompleted()
   {
      PublishChangesToQueue();
      
      if (m_object.RefCount() == 2)
      {
         // About to become single reference again
         m_mutex->unlock();
      }
   }

   RestApiSecurityLevel::Value GenericResource::GetAccessLevel(CRUD crud) const
   {
      switch (crud)
      {
      case CRUD::CREATE: return jude_user_Root;
      case CRUD::READ:   return m_access.canRead;
      case CRUD::UPDATE: return m_access.canUpdate;
      case CRUD::DELETE: return jude_user_Root;
      }

      return jude_user_Root;
   }

   void GenericResource::SetAccessLevel(CRUD crud, RestApiSecurityLevel::Value level)
   {
      switch (crud)
      {
      case CRUD::CREATE: /* not allowed to change*/  break;
      case CRUD::READ:   m_access.canRead   = level; break;
      case CRUD::UPDATE: m_access.canUpdate = level; break;
      case CRUD::DELETE: /* not allowed to change*/  break;
      }
   }

   void GenericResource::ClearAllDataAndSubscribers()
   {
      m_subscribers.clear();
      m_validators.clear();
      m_object.Clear();
   }

   ValidationResult GenericResource::Validate(Validation<Object>& info)
   {
      for (auto& validator : m_validators)
      {
         auto result = validator.second(info);
         if (!result)
         {
            return result;
         }
      }
      return true;
   }

   SubscriptionHandle GenericResource::ValidateWith(Validatable<>::Validator callback)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      auto validateorId = ++nextSubscriberId;
      m_validators[validateorId] = callback;

      return SubscriptionHandle([=] { DeregisterValidator(validateorId); });
   }

   SubscriptionHandle GenericResource::SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue)
   {
      return OnChangeToPath(
         "", 
         [=] (const Notification<Object>& notification) {
            callback(prefix, notification);
         },
         filterGenerator(*GetType()),
         queue);
   }

   bool GenericResource::Restore(std::string, std::istream& input)
   {
      return RestPut("", input, jude_user_Root).IsOK();
   }

   void GenericResource::DeregisterValidator(uint32_t validateorId)
   {
      m_validators.erase(validateorId);
   }

   static void AddSchemas(std::set<const jude_rtti_t*>& schemas, const jude_rtti_t* rtti)
   {
      schemas.insert(rtti);

      for (jude_size_t index = 0; index < rtti->field_count; index++)
      {
         if (jude_field_is_object(&rtti->field_list[index]))
         {
            AddSchemas(schemas, rtti->field_list[index].details.sub_rtti);
         }
      }
   }

   void GenericResource::OutputAllSchemasInYaml(std::ostream& output, std::set<const jude_rtti_t*>& alreadyDone, RestApiSecurityLevel::Value userLevel) const
   {
      swagger::RecursivelyOutputSchemas(output, alreadyDone, &m_object.Type(), userLevel);
   }

   void GenericResource::OutputAllSwaggerPaths(std::ostream& output, const std::string& prefix, RestApiSecurityLevel::Value userLevel) const
   {
      const auto& rtti = m_object.Type();

      // output swagger end points descriptions for the entire resource...
      auto resourceName = m_name.c_str();
      output << "  " << prefix << "/" << resourceName << "/:";

      std::string apiTag = prefix + "/" + m_name;

      char buffer[1024];
      if (userLevel >= m_access.canRead)
      {
         snprintf(buffer, std::size(buffer), swagger::GetTemplate, resourceName, apiTag.c_str(), rtti.name, rtti.name);
         output << buffer;
      }
      if (userLevel >= m_access.canUpdate)
      {
         snprintf(buffer, std::size(buffer), swagger::PatchTemplate, resourceName, apiTag.c_str(), rtti.name, rtti.name);
         output << buffer;

         // PUT is technically optional but discouraged as it clears other writable fields
         //output.Printf(1024, swagger::PutTemplate, resourceName, resourceName, rtti.name, rtti.name);

         // output endpoints for any action fields
         for (jude_index_t index = 0; index < rtti.field_count; ++index)
         {
            auto field = rtti.field_list[index];
            if (!field.is_action)
            {
               continue;
            }
            auto schema = swagger::GetSchemaForActionField(field, userLevel);

            output << "\n  " << prefix << '/' <<  resourceName << '/' << field.label << ':';
            snprintf(buffer, std::size(buffer), swagger::PatchActionTemplate, field.label, resourceName, apiTag.c_str(), schema.c_str(), rtti.name);
            output << buffer;
         }
      }
   }

   std::string GenericResource::GetSwaggerReadSchema(RestApiSecurityLevel::Value userLevel) const
   {
      if (userLevel < m_access.canRead)
      {
         return "";
      }

      std::stringstream output;
      output << "        " << m_name << ":\n"
             << "          $ref: '#/components/schemas/" << m_object.Type().name << "_Schema'\n";
      return output.str();
   }

   RestfulResult GenericResource::OnTransactionCompleted(Object& copy, bool needsCommit)
   {
      if (!(needsCommit && copy.IsOK() && copy.IsChanged()))
      {
         // no copy (abandoned transaction) or no changes... 
         return jude_rest_OK;
      }

      string errorMsg;
      RestfulResult result = jude_rest_OK;
      
      // always validate even if we are not ready to commit
      Validation<Object> info(copy, [&] { return this->m_object; });
      auto isValid = Validate(info);
      if (!isValid)
      {
         result = RestfulResult(jude_rest_Bad_Request, std::move(isValid.error));
      }  

      if (result)
      {
         // now replace the old resource
         m_object.TransferFrom(std::move(copy));
      }

      PublishChangesToQueue();

      return result;
   }

   void GenericResource::PublishChangesToQueue()
   {
      Notification<Object> notification(m_object, [&] { return GenericLock(); });
      m_object.ClearChangeMarkers();
      set<NotifyQueue*> queues;

      auto changes = notification.GetChangeMask();
      for (auto& entry : m_subscribers)
      {
         auto& subscriber = entry.second;
         if (  queues.find(subscriber.queue) == queues.end()
            && (subscriber.filter && changes)
            )
         {
            if (subscriber.queue->IsImmediate())
            {
               subscriber.callback(notification);
            }
            else
            {
               // not already queued and filter is overlapping
               queues.insert(subscriber.queue);
               auto origin = subscriber.queue;
               subscriber.queue->Send([=] { HandleChangesFromQueue(notification, origin); });
            }
         }
      }
   }

   void GenericResource::HandleChangesFromQueue(const Notification<Object> notification, NotifyQueue *origin)
   {
      for (auto& entry : m_subscribers)
      {
         auto& subscriber = entry.second;
         if (subscriber.queue == origin    // waitng on same queue
            && (subscriber.filter && notification->GetChanges()) // filter overlaps
            ) 
         {
            subscriber.callback(notification);
         }
      }      
   }

   Object GenericResource::GenericLock()
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      if (m_object.RefCount() == 1)
      {
         // First time lock...
         m_mutex->lock();
      }

      return m_object;
   }

   Transaction<Object> GenericResource::GenericTransaction()
   {
      // Lock now - pass this into the transaction to keep locked until the transaction completes.
      std::unique_lock<jude::Mutex> lock(*m_mutex);

      return Transaction<Object>(
         std::move(lock),
         m_object,
         [&](Object& resource, bool needsCommit) -> RestfulResult
         {
            return OnTransactionCompleted(resource, needsCommit);
         });
   }

   std::vector<std::string> GenericResource::SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, RestApiSecurityLevel::Value userLevel) const
   {
      return m_object.SearchForPath(operationType, pathPrefix, maxPaths, userLevel);
   }

   RestfulResult GenericResource::PatchObject(Object& newObject)
   {
      auto transaction = GenericTransaction();
      transaction->Patch(newObject); // apply deltas in new resource only
      return transaction.Commit();
   }

   RestfulResult GenericResource::PutObject(Object& newObject)
   {
      auto transaction = GenericTransaction();
      transaction->Put(newObject); // apply deltas in new resource only
      return transaction.Commit();
   }

   RestfulResult GenericResource::RestGet(const char* fullpath, std::ostream& output, const AccessControl& accessControl) const
   {
      if (accessControl.GetAccessLevel() < m_access.canRead)
      {
         return jude_rest_Forbidden;
      }

      return m_object.Clone().RestGet(fullpath, output, accessControl);
   }

   RestfulResult ApplyAndCommit(Transaction<Object>& transaction, RestfulResult initialResult)
   {
      if (!initialResult)
      {
         transaction.Abort();
         return initialResult;
      }
      return transaction.Commit();
   }

   RestfulResult GenericResource::RestPost(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (GetNextUrlToken(fullpath, nullptr).length() == 0)
      {
         return RestfulResult(jude_rest_Method_Not_Allowed, "Cannot POST to root of permanent resource");
      }

      auto transaction = GenericTransaction();
      return ApplyAndCommit(transaction, transaction->RestPost(fullpath, input, accessControl));
   }

   RestfulResult GenericResource::RestPatch(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (accessControl.GetAccessLevel() < m_access.canUpdate)
      {
         return jude_rest_Forbidden;
      }

      auto transaction = GenericTransaction();
      return ApplyAndCommit(transaction, transaction->RestPatch(fullpath, input, accessControl));
   }

   RestfulResult GenericResource::RestPut(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (accessControl.GetAccessLevel() < m_access.canUpdate)
      {
         return jude_rest_Forbidden;
      }

      auto transaction = GenericTransaction();
      return ApplyAndCommit(transaction, transaction->RestPut(fullpath, input, accessControl));
   }

   RestfulResult GenericResource::RestDelete(const char* fullpath, const AccessControl& accessControl)
   {
      if (GetNextUrlToken(fullpath, nullptr).length() == 0)
      {
         return RestfulResult(jude_rest_Method_Not_Allowed, "Cannot DELETE a permanent resource");
      }

      auto transaction = GenericTransaction();
      return ApplyAndCommit(transaction, transaction->RestDelete(fullpath, accessControl));
   }

   string GenericResource::DebugInfo() const
   {
      string info = "Resource resource: " + m_name + "\n";
      info += m_object.DebugInfo();
      return info;
   }

   SubscriptionHandle GenericResource::OnChangeToPath(
      const std::string& subscriptionPath,
      Subscriber callback,
      FieldMask resourceFieldFilter,
      NotifyQueue& queue)
   {
      const jude_field_t *field = nullptr; 
      if (subscriptionPath.length() != 0)
      {
         field = jude_rtti_find_field(GetType(), subscriptionPath.c_str());
         if (field == nullptr)
         {
            jude_debug("ERROR: Cannot subscribe further into individual resource with path '%s'", subscriptionPath.c_str());
            return nullptr;
         }
      }

      std::lock_guard<jude::Mutex> lock(*m_mutex);
      
      jude::FieldMask filter(resourceFieldFilter);
      if (field)
      {
         filter.ClearAll();
         filter.SetChanged(field->index);
      }

      auto subscriberId = ++nextSubscriberId;
      m_subscribers[subscriberId] = { filter, callback, &queue };

      return SubscriptionHandle([this,subscriberId] { Unsubscribe(subscriberId); });
   }

   void GenericResource::Unsubscribe(uint32_t subscriberId)
   {
      lock_guard<jude::Mutex> lock(*m_mutex);
      m_subscribers.erase(subscriberId);
   }

}
