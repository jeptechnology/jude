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

#include <jude/database/Collection.h>
#include <jude/database/Swagger.h>
#include <jude/jude.h>
#include <utility>
#include <algorithm>
#include <inttypes.h>

using namespace std;

namespace jude
{
   CollectionBase::CollectionBase(const jude_rtti_t& RTTI, const std::string& name, jude_user_t accessLevel, size_t capacity, std::shared_ptr<jude::Mutex> mutex)
      : DatabaseEntry(mutex)
      , m_rtti(RTTI)
      , m_name(name)
      , m_capacity(capacity)
   {
      m_access.canCreate = accessLevel;
      m_access.canRead   = accessLevel;
      m_access.canUpdate = accessLevel;
      m_access.canDelete = accessLevel;
   }

   jude_user_t CollectionBase::GetAccessLevel(CRUD crud) const
   {
      switch (crud)
      {
      case CRUD::CREATE: return m_access.canCreate;
      case CRUD::READ:   return m_access.canRead;
      case CRUD::UPDATE: return m_access.canUpdate;
      case CRUD::DELETE: return m_access.canDelete;
      }
      return jude_user_Root;
   }

   void CollectionBase::SetAccessLevel(CRUD crud, jude_user_t level)
   {
      switch (crud)
      {
      case CRUD::CREATE: m_access.canCreate = level; break;
      case CRUD::READ:   m_access.canRead   = level; break;
      case CRUD::UPDATE: m_access.canUpdate = level; break;
      case CRUD::DELETE: m_access.canDelete = level; break;
      }
   }

   void CollectionBase::OutputAllSchemasInYaml(std::ostream& output, std::set<const jude_rtti_t*>& alreadyDone, jude_user_t userLevel) const
   {
      swagger::RecursivelyOutputSchemas(output, alreadyDone, &m_rtti, userLevel);
   }

   void CollectionBase::OutputAllSwaggerPaths(std::ostream& output, const std::string& prefix, jude_user_t userLevel) const
   {
      char buffer[1024];
      auto collectionName = m_name.c_str();

      std::string apiTag = prefix + "/" + m_name;

      // output collection endpoints
      output << "  " << prefix << '/' << collectionName << "/:";
      
      if (userLevel >= m_access.canRead)
      {
         snprintf(buffer, std::size(buffer), swagger::GetAllTemplate, collectionName, apiTag.c_str(), m_rtti.name, m_rtti.name);
         output << buffer;
      }
      if (userLevel >= m_access.canCreate)
      {
         snprintf(buffer, std::size(buffer), swagger::PostTemplate, collectionName, apiTag.c_str(), m_rtti.name, m_rtti.name);
         output << buffer;
      }

      // output resource endpoints
      output << "\n  " << prefix << '/' << collectionName << "/{id}:";
      if (userLevel >= m_access.canRead)
      {
         snprintf(buffer, std::size(buffer), swagger::GetWithIdTemplate, collectionName, apiTag.c_str(), m_rtti.name, m_rtti.name);
         output << buffer;
      }
      if (userLevel >= m_access.canUpdate)
      {
         snprintf(buffer, std::size(buffer), swagger::PatchWithIdTemplate, collectionName, apiTag.c_str(), m_rtti.name, m_rtti.name);
         output << buffer;
         // PUT is technically optional but discouraged as it clears other writable fields
         // ... swagger::PutWithIdTemplate, collectionName, collectionName, m_rtti.name, m_rtti.name ...
      }
      if (userLevel >= m_access.canDelete)
      {
         snprintf(buffer, std::size(buffer), swagger::DeleteWithIdTemplate, collectionName, apiTag.c_str(), m_rtti.name, m_rtti.name);
         output << buffer;
      }

      if (userLevel >= m_access.canUpdate)
      {
         // output endpoints for any action fields
         for (jude_index_t index = 0; index < m_rtti.field_count; ++index)
         {
            auto field = m_rtti.field_list[index];
            if (!field.is_action)
            {
               continue;
            }
            auto schema = swagger::GetSchemaForActionField(field, userLevel);
            snprintf(buffer, std::size(buffer), "\n  %s/%s/{id}/%s:", prefix.c_str(), collectionName, field.label);
            output << buffer;
            snprintf(buffer, std::size(buffer), swagger::PatchActionWithIdTemplate, field.label, collectionName, apiTag.c_str(), schema.c_str(), m_rtti.name);
            output << buffer;
         }
      }
   }

   std::string CollectionBase::GetSwaggerReadSchema(jude_user_t userLevel) const
   {
      if (userLevel < m_access.canRead)
      {
         return "";
      }

      static const char* schemaTemplateMap = "        %s:\n"
                                             "          type: object\n"
                                             "          additionalProperties:\n"
                                             "            $ref: '#/components/schemas/%s_Schema'\n";

      static const char* schemaTemplateArray = "        %s:\n"
                                               "          type: array\n"
                                               "          items:\n"
                                               "            $ref: '#/components/schemas/%s_Schema'\n";

      char buffer[1024];
      snprintf(buffer, std::size(buffer), (Options::SerialiseCollectionAsObjectMap ? schemaTemplateMap : schemaTemplateArray), m_name.c_str(), m_rtti.name);
      return buffer;
   }

   ValidationResult CollectionBase::Validate(Validation<Object>& info)
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

   // These *must* be called symmetrically
   Object CollectionBase::LockForEdit(jude_id_t id, bool next)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      auto it = next ?
           (id == JUDE_INVALID_ID ? m_objects.begin() : m_objects.upper_bound(id))
         : (m_objects.find(id));

      if (it == m_objects.end())
      {
         return nullptr;
      }

      if (it->second.RefCount() == 1)
      {
         // The only reference would be that held in the collection.
         // Here, we lock again so this resolurce is locked "outside" the collection
         // until such time as reference count get back to one - then it is "editCompleted" and unlocked
         m_mutex->lock(); 
      }

      return it->second;
   }
   
   void CollectionBase::OnEdited(jude_id_t id)
   {
      if (Options::NotifyImmediatelyOnChange)
      {
         PublishChangesToQueue(id);
      }
   }

   void CollectionBase::OnEditCompleted(jude_id_t id)
   {
      // Now that the editing is completed, publish any changes
      PublishChangesToQueue(id);

      m_mutex->unlock();
   }

   RestfulResult CollectionBase::PostObject(const Object& object)
   {
      if (auto transaction = CreatePostTransaction())
      {
         transaction->Patch(object);
         return transaction.Commit();
      }
      return RestfulResult(jude_rest_Bad_Request, "Could not create new schedule");
   }

   RestfulResult CollectionBase::PatchObject(const Object& object)
   {
      if (!object.IsIdAssigned())
      {
         return RestfulResult(jude_rest_Bad_Request, "Can't PATCH object with unknown ID");
      }

      if (auto transaction = LockForTransaction(object.Id()))
      {
         transaction->Patch(object);
         return transaction.Commit();
      }
      return jude_rest_Not_Found;
   }

   RestfulResult CollectionBase::OnTransactionCompleted(jude_id_t id, Object& editedCopy, bool needsCommit)
   {
      if (!needsCommit || !editedCopy.IsChanged())
      {
         return jude_rest_OK;
      }

      if (!editedCopy)
      {
         return jude_rest_Internal_Server_Error;
      }

      auto objectRecord = m_objects.find(id);
      if (objectRecord == m_objects.end())
      {
         // Already removed, we can't commit!
         return jude_rest_Internal_Server_Error;
      }
      if (editedCopy.Id() != id)
      {
         jude_debug("WARNING: Transaction attempted change of id to %" PRIjudeID " - restting it to %" PRIjudeID, editedCopy.Id(), id);
         editedCopy.AssignId(id);
      }

      Validation<> validation(&editedCopy, [&] { return objectRecord->second; }, false);
      auto result = Validate(validation);
      if (!result)
      {
         return RestfulResult(jude_rest_Bad_Request, result.error);
      }

      objectRecord->second = editedCopy;
      PublishChangesToQueue(objectRecord->second, false);

      return jude_rest_OK;
   }

   bool CollectionBase::ContainsId(jude_id_t id) const
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      return  m_objects.find(id) != m_objects.end();
   }

   std::vector<jude_id_t> CollectionBase::GetIds() const
   {
      std::vector<jude_id_t> idList;
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      for (auto const &it : m_objects)
      {
         idList.push_back(it.first);
      }
      return idList;
   }

   jude_id_t CollectionBase::FindObjectIdFromPath(const char* path_token) const
   {
      if (path_token == nullptr)
      {
         return JUDE_INVALID_ID;
      }

      if (path_token[0] != '*')
      {
         char* endptr = nullptr;
         auto id = strtoull(path_token, &endptr, 10);
         if (errno == 0 && path_token && !*endptr)
         {
            return (jude_id_t)id;
         }
         return JUDE_INVALID_ID;
      }

      path_token++;

      char key[64];
      auto* value = strchr(path_token, '=');
      if (value == NULL)
      {
         return JUDE_INVALID_ID; // no '=' after 'key'
      }

      size_t key_length = value - path_token;
      if (key_length >= sizeof(key))
      {
         return JUDE_INVALID_ID; // key too long
      }

      strncpy(key, path_token, key_length);
      key[key_length] = 0;

      value++;
      if (value[0] == '\0')
      {
         return JUDE_INVALID_ID; // no value
      }
      std::string searchValue = value;

      const jude_field_t* key_field = jude_rtti_find_field(&m_rtti, key);
      if (!key_field)
      {
         return JUDE_INVALID_ID;
      }

      for (auto& resource : m_objects)
      {
         if (searchValue == resource.second.GetFieldAsString(key_field->index))
         {
            return resource.second.Id();
         }
      }

      return JUDE_INVALID_ID;
   }

   Object CollectionBase::LockForEditFromPath(const char** fullpath, bool& isRootPath)
   {
      // Find the token...
      auto token = RestApiInterface::GetNextUrlToken(*fullpath, fullpath); // Note: updates fullpath to next token...
      if (token.size() == 0)
      {
         isRootPath = true;  
         return nullptr;
      }

      isRootPath = false;

      // parse the id value
      auto id = FindObjectIdFromPath(token.c_str());
      return LockForEdit(id);
   }


   Transaction<Object> CollectionBase::CreateTransactionFromPath(const char** fullpath, bool& isRootPath)
   {
      // Find the token...
      auto token = RestApiInterface::GetNextUrlToken(*fullpath, fullpath); // Note: updates fullpath to next token...
      if (token.size() == 0)
      {
         isRootPath = true;
         return CreatePostTransaction();
      }

      isRootPath = false;

      // parse the id value
      jude_id_t id = FindObjectIdFromPath(token.c_str());
      return LockForTransaction(id);
   }

   std::vector<std::string> CollectionBase::SearchForPath(CRUD operationType, const char* pathPrefix, jude_size_t maxPaths, jude_user_t userLevel) const
   {
      pathPrefix = pathPrefix ? pathPrefix : "";

      if (pathPrefix[0] == '\0' || pathPrefix[0] != '/')
      {
         return {};
      }

      const char* remainingSuffix;
      std::string token = RestApiInterface::GetNextUrlToken(pathPrefix, &remainingSuffix, false);

      if (strlen(remainingSuffix) == 0)
      {
         std::vector<std::string> paths;
         char buffer[32];
         for (const auto& resource : m_objects)
         {
            snprintf(buffer, sizeof(buffer), "%" PRIjudeID, resource.first);
            if (0 == strncmp(buffer, token.c_str(), token.length()))
            {
               paths.push_back("/" + token + (buffer + token.length()));
            }
         }
         return paths;
      }
      else
      {
         jude_id_t id = FindObjectIdFromPath(token.c_str());
         const auto ref = GenericLock(id);
         if (!ref)
         {
            return {};
         }

         std::vector<std::string> paths;
         auto subpaths = ref->SearchForPath(operationType, remainingSuffix, maxPaths, userLevel);
         for (auto& subpath : subpaths)
         {
            paths.push_back("/" + token + subpath);
         }
         return paths;
      }
   }

   RestfulResult CollectionBase::Post(const Object& newObject, bool generate_uuid, bool andValidate)
   {
      auto candidateObject = newObject.Clone(false);
      if (generate_uuid || !candidateObject.IsIdAssigned())
      {
         candidateObject.AssignId(jude_generate_uuid());
      }

      auto uuid = candidateObject.Id();

      if (andValidate)
      {
         // NOTE: Validation object can update the candidateObject in order to fix up any default values or status 
         // Also, some validators need to know if this is a new candidate object so mark it as new
         candidateObject.MarkObjectAsNew(); 
         Validation<Object> info(&candidateObject, {}, false);
         auto isValid = Validate(info);
         if (!isValid)
         {
            return RestfulResult(jude_rest_Bad_Request, std::move(isValid.error));
         }
      }

      // It's valid - add it to our list
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      if (Full())
      {
         return RestfulResult(jude_rest_Bad_Request, "Collection '" + m_name + "' is full");
      }

      auto &storedObject = m_objects[uuid];
      // We create our final clone of the candidate storedObject here with the correct callbacks for editing
      // NOTE: move-assingment to prevent the callbacks 
      storedObject = std::move(candidateObject.Clone(false, 
                                                [this, uuid] { OnEdited(uuid); },          // This may be called on each change to an object
                                                [this, uuid] { OnEditCompleted(uuid); })); // This will be called when the refcount of the object is back to 1

      storedObject.MarkObjectAsNew(); // a posted object is always "new"
      PublishChangesToQueue(storedObject, false); // We've just been added
      storedObject.ClearChangeMarkers();

      return RestfulResult(uuid);
   }

   RestfulResult CollectionBase::RestoreEntry(std::istream& input)
   {
      Object restoredObject(m_rtti);
      restoredObject.RestPut("", input);
      return Post(std::move(restoredObject), false, false); 
   }

   RestfulResult CollectionBase::Delete(jude_id_t id)
   {
      auto resource = LockForEdit(id);
      if (!resource)
      {
         return jude_rest_Not_Found;
      }

      // Is it valid to delete this resource?
      auto isDeleted = true;
      Validation<Object> info(resource, {}, isDeleted);
      auto isValid = Validate(info);
      if (!isValid)
      {
         return RestfulResult(jude_rest_Bad_Request, std::move(isValid.error));
      }

      m_objects.erase(id);

      PublishChangesToQueue(resource, true);
      
      return jude_rest_No_Content;
   }

   void CollectionBase::PublishChangesToQueue(jude_id_t id)
   {
      auto ref = m_objects.find(id);
      bool isDeleted = (ref == m_objects.end());
      if (isDeleted)
      {
         return;
      }

      PublishChangesToQueue(ref->second, false);
   }

   void CollectionBase::PublishChangesToQueue(Object& changedObject, bool isDeleted)
   {  
      auto id = changedObject.Id();

      // Create a Notification object - this makes a read copy that captures the change markers...
      Notification<Object> event(changedObject, [&, id] { return *GenericLock(id); }, isDeleted);
      // Now we can clear the change markers of the underlying object before notifying - this helps prevent gratuitous notifications
      changedObject.ClearChangeMarkers();

      set<NotifyQueue*> queues;

      auto changes = event.GetChangeMask();
      for (auto& entry : m_subscribers)
      {
         auto& subscriber = entry.second;
         if (   queues.find(subscriber.queue) == queues.end()
            && (subscriber.id == JUDE_AUTO_ID || subscriber.id == id)
            && (subscriber.filter && changes))
         {
            if (subscriber.queue->IsImmediate())
            {
               subscriber.callback(event);
            }
            else
            {
               // not already queued and filter is overlapping
               queues.insert(subscriber.queue);
               auto origin = subscriber.queue;
               subscriber.queue->Send([=] { HandleChangesFromQueue(event, origin); });
            }
         }
      }
   }

   void CollectionBase::HandleChangesFromQueue(const Notification<Object>& notification, NotifyQueue* origin)
   {
      for (auto& entry : m_subscribers)
      {
         auto& subscriber = entry.second;
         if (subscriber.queue == origin    // waitng on same queue
            && (subscriber.filter && notification->GetChanges())) // filter overlaps
         {
            subscriber.callback(notification);
         }
      }
   }

   void CollectionBase::clear()
   {
      std::vector<jude_id_t> ids;

      {
         std::lock_guard<jude::Mutex> lock(*m_mutex);
         for (const auto& id : m_objects)
         {
            ids.push_back(id.first);
         }
      }

      for (const auto& id : ids)
      {
         Delete(id);
      }
   }

   void CollectionBase::ClearAllDataAndSubscribers()
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);

      m_subscribers.clear();
      m_validators.clear();

      clear(); 
   }

   size_t CollectionBase::count() const
   {
      return m_objects.size();
   }

   RestfulResult CollectionBase::RestGet(const char* fullpath, std::ostream& output, const AccessControl& accessControl) const
   {
      if (accessControl.GetAccessLevel() < m_access.canRead)
      {
         return jude_rest_Forbidden;
      }

      bool isRootPath;
      auto resource = const_cast<CollectionBase*>(this)->LockForEditFromPath(&fullpath, isRootPath).Clone();

      if (isRootPath) // no id token given
      {         
         output << (Options::SerialiseCollectionAsObjectMap ? "{" : "[");

         bool commaNeeded = false;

         // Get all resources in the collection...
         for (const auto& resource : m_objects)
         {
            if (commaNeeded)
            {
               output << ',';
            }
            else
            {
               commaNeeded = true;
            }

            if (Options::SerialiseCollectionAsObjectMap)
            {
               output << '"' << resource.first << "\":";
            }

            auto result = resource.second.RestGet("/", output, accessControl);
            if (!result)
            {
               return result;
            }
         }

         output << (Options::SerialiseCollectionAsObjectMap ? "}" : "]");

         return jude_rest_OK;
      }
      else if (resource)
      {
         // Get a single resource
         return resource.RestGet(fullpath, output, accessControl);
      }

      return jude_rest_Not_Found;
   }

   RestfulResult CollectionBase::RestPost(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      bool isRootPath;
      auto transaction = CreateTransactionFromPath(&fullpath, isRootPath);

      if (isRootPath && accessControl.GetAccessLevel() < GetAccessLevel(CRUD::CREATE))
      {
         return jude_rest_Forbidden;
      }

      if (!transaction)
      {
         return jude_rest_Not_Found;
      }

      auto result = isRootPath ? transaction->RestPut("", input, accessControl)     // for root objects, we "put" the stream into a new object
                               : transaction->RestPost("", input, accessControl);   // otherwise we "post" into an existing object
      if (!result)
      {
         transaction.Abort();
         return result;
      }
      return transaction.Commit();
   }

   RestfulResult CollectionBase::RestPatch(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (accessControl.GetAccessLevel() < m_access.canUpdate)
      {
         return jude_rest_Forbidden;
      }

      bool isRootPath;
      auto transaction = CreateTransactionFromPath(&fullpath, isRootPath);

      if (!transaction)
      {
         return jude_rest_Not_Found;
      }

      if (isRootPath) // no id token given
      {
         transaction.Abort();
         return RestfulResult(jude_rest_Method_Not_Allowed, "Cannot PATCH to root of collection");
      }
      
      auto result = transaction->RestPatch(fullpath, input, accessControl);
      if (!result)
      {
         transaction.Abort();
         return result;
      }

      return transaction.Commit();
   }

   RestfulResult CollectionBase::RestPut(const char* fullpath, std::istream& input, const AccessControl& accessControl)
   {
      if (accessControl.GetAccessLevel() < m_access.canUpdate)
      {
         return jude_rest_Forbidden;
      }

      bool isRootPath;
      auto transaction = CreateTransactionFromPath(&fullpath, isRootPath);

      if (!transaction)
      {
         return jude_rest_Not_Found;
      }

      if (isRootPath) // no id token given
      {
         transaction.Abort();
         return RestfulResult(jude_rest_Method_Not_Allowed, "Cannot PATCH to root of collection");
      }
      
      auto result = transaction->RestPut(fullpath, input, accessControl);
      if (!result)
      {
         transaction.Abort();
         return result;
      }

      return transaction.Commit();
   }

   RestfulResult CollectionBase::RestDelete(const char* fullpath, const AccessControl& accessControl)
   {
      bool isRootPath;
      auto transaction = CreateTransactionFromPath(&fullpath, isRootPath);

      if (!transaction)
      {
         return jude_rest_Not_Found;
      }

      if (isRootPath) // no id token given
      {
         transaction.Abort();
         return RestfulResult(jude_rest_Method_Not_Allowed, "Cannot DELETE entire collection");
      }

      // Are we deleting the entire resource or just a part of the internals?
      if (RestApiInterface::GetNextUrlToken(fullpath).size() == 0)
      {
         transaction.Abort();

         if (accessControl.GetAccessLevel() < m_access.canDelete)
         {
            return jude_rest_Forbidden;
         }

         // Delete this id
         return Delete(transaction->Id());
      }

      // Delete something inside the resource (e.g. a field or an element from an array
      auto result = transaction->RestDelete(fullpath, accessControl);
      if (!result)
      {
         return result;
      }

      return transaction.Commit();
   }

   // From PubSubInterface<>
   SubscriptionHandle CollectionBase::OnChangeToPath(
      const std::string& subscriptionPath,
      Subscriber callback,
      FieldMask resourceFieldFilter,
      NotifyQueue& queue)
   {
      jude_id_t id = JUDE_AUTO_ID; // assume wildcard - all id's in collection
      const jude_field_t *field = nullptr; 
      
      if (subscriptionPath.length() != 0)
      {
         const char *fieldName = nullptr;
         // expect a numeric id or a '+' for wildcard
         auto idString = GetNextUrlToken(subscriptionPath.c_str(), &fieldName);
         if (idString != "+")
         {
            id = std::strtoul(idString.c_str(), nullptr, 10);
         }

         // did we also supply a field name?
         if (fieldName)
         {
            field = jude_rtti_find_field(GetType(), fieldName);
            if (field == nullptr)
            {
               jude_debug("ERROR: Cannot subscribe further into individual collection with path '%s'", subscriptionPath.c_str());
               return nullptr;
            }
         }
      }
     
      jude::FieldMask filter(resourceFieldFilter);
      if (field)
      {
         filter.ClearAll();
         filter.SetChanged(field->index);
         // Should we notify when the resource gets removed?
         filter.SetChanged(JUDE_ID_FIELD_INDEX); // If we specify a field, then also inform if the resource is deleted from collection
      }

      std::lock_guard<jude::Mutex> lock(*m_mutex);
      auto subscriberId = ++nextSubscriberId;
      m_subscribers[subscriberId] = { filter, callback, &queue, id };

      return SubscriptionHandle([=] { Unsubscribe(subscriberId); });
   }

   void CollectionBase::Unsubscribe(uint32_t subscriberId)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      m_subscribers.erase(subscriberId);
   }

   SubscriptionHandle CollectionBase::ValidateWith(Validatable<>::Validator callback)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      auto validatorId = ++nextSubscriberId;
      m_validators[validatorId] = callback;

      return SubscriptionHandle([this, validatorId] { DeregisterValidator(validatorId); });
   }

   void CollectionBase::DeregisterValidator(uint32_t validatorId)
   {
      std::lock_guard<jude::Mutex> lock(*m_mutex);
      m_validators.erase(validatorId);
   }

   SubscriptionHandle CollectionBase::SubscribeToAllPaths(std::string prefix, PathNotifyCallback callback, FieldMaskGenerator filterGenerator, NotifyQueue& queue)
   {
      return OnChangeToPath(
         "", 
         [=] (const Notification<Object>& notification) {
            callback(prefix + "/" + std::to_string(notification->Id()), notification);
         },
         filterGenerator(*GetType()),
         queue);
   }

   bool CollectionBase::Restore(std::string, std::istream& input)
   {
      return RestoreEntry(input).IsOK();
   }

   std::string DebugInfoForFilter(const FieldMask& filter)
   {
      std::string info = "[";
      for (int bit = 0; bit < 16; bit++)
      {
         if (filter.IsChanged(bit))
         {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d ", bit);
            info += tmp;
         }
      }
      info += "]";
      return info;
   }

   std::string CollectionBase::DebugInfo() const
   {
      std::string info = "CollectionBase " + m_name + ":\n";
      
      info += "Objects: [\n";
      for (const auto& resource : m_objects)
      {
         char tmp[32];
         snprintf(tmp, sizeof(tmp), "%" PRIjudeID " :\n", resource.first);
         info += tmp;
         info += resource.second.DebugInfo();
         info += "\n";
      }
      info += "\n]\n";

      info += "CollectionBase Subscribers: {\n";
      for (const auto& subscriber : m_subscribers)
      {
         info += "subscriber filter: ";
         info += DebugInfoForFilter(subscriber.second.filter.Get()).c_str();
         info += "\n";
      }
      info += "}\n";

      info += "]\n";

      return info;
   }
}
