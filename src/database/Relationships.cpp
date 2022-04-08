
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

#include <jude/database/Relationships.h>

namespace jude 
{
   namespace detail
   {
      void CheckForDeletion(const Notification<Object>& notification, Relationships::ReferenceId reference)
      {
         std::vector<jude_id_t> idsToDelete;
         // find all resources that reference the id that has just been deleted
         for (auto& resource : reference.collection)
         {
            if (!resource.Has(reference.fieldIndex))
               continue;

            auto referenceId = resource.GetFieldAsNumber<jude_id_t>(reference.fieldIndex);
            if (referenceId == notification->Id())
            {
               idsToDelete.push_back(resource.Id());
            }
         }

         for (auto& id : idsToDelete)
         {
            reference.collection.Delete(id);
         }            
      }

      std::vector<jude_id_t> GetAllValues(const Object& resource, jude_index_t fieldIndex)
      {
         std::vector<jude_id_t> values;
         for (jude_index_t arrayIndex = 0; arrayIndex < resource.CountField(fieldIndex); arrayIndex++)
         {
            values.push_back(resource.GetFieldAsNumber<jude_id_t>(fieldIndex, arrayIndex));
         }
         return values;
      }

      ValidationResult ValidateReference(
         Notification<Object>& changedObject, 
         Relationships::ReferenceId referencePath, 
         CollectionBase& targetCollection,
         bool allowMulitpleReferencesToOneTarget)
      {
         if (changedObject.IsDeleted() || !changedObject->IsChanged(referencePath.fieldIndex))
         { 
            return true; // not interested in deleted resource or if the path field is not changed
         }

         std::set<int64_t> changedIdList;

         for (auto& id : GetAllValues(*changedObject, referencePath.fieldIndex))
         {
            if (!targetCollection.ContainsId(id))
            {
               // changed id is reference to non-existent resource in target collection
               char errorBuffer[128];
               std::string idString = changedObject.IsNew() ? "<new>" : std::to_string(changedObject->Id()).c_str();
               snprintf(errorBuffer, sizeof(errorBuffer), 
                        "'%s/%s/%s' refers to id %" PRIjudeID " which is not in collection '%s'",
                        referencePath.collection.GetName().c_str(),
                        idString.c_str(),
                        changedObject->Type().field_list[referencePath.fieldIndex].label,
                        id,
                        targetCollection.GetName().c_str());
               
               return errorBuffer;
            }

            if (changedIdList.find(id) != changedIdList.end())
            {
               // duplicate id in the list
               char errorBuffer[128];
               std::string idString = changedObject.IsNew() ? "<new>" : std::to_string(changedObject->Id()).c_str();
               snprintf(errorBuffer, sizeof(errorBuffer),
                  "'%s/%s/%s' has duplicate entry %" PRIjudeID,
                  referencePath.collection.GetName().c_str(),
                  idString.c_str(),
                  changedObject->Type().field_list[referencePath.fieldIndex].label,
                  id);
               return errorBuffer;
            }

            changedIdList.insert(id);
         }

         if (allowMulitpleReferencesToOneTarget)
         {
            return true;
         }

         // Check for duplicate references in other members of reference collection
         for (const auto& other : referencePath.collection)
         {
            if (other.Id() == changedObject->Id())
            {
               continue;
            }

            for (auto& id : GetAllValues(other, referencePath.fieldIndex))
            {
               if (changedIdList.find(id) != changedIdList.end())
               {
                  char errorBuffer[128];
                  std::string idString = changedObject.IsNew() ? "<new>" : std::to_string(changedObject->Id()).c_str();
                  snprintf(errorBuffer, sizeof(errorBuffer),
                     "'%s/%s/%s' and '%s/%" PRIjudeID "/%s' have duplucate id: %" PRIjudeID,
                     referencePath.collection.GetName().c_str(),
                     idString.c_str(),
                     changedObject->Type().field_list[referencePath.fieldIndex].label,
                     referencePath.collection.GetName().c_str(),
                     other.Id(),
                     changedObject->Type().field_list[referencePath.fieldIndex].label,
                     id);
                  return errorBuffer;
               }
            }
         }
         return true;
      }

      void ClearReferenceFieldIfNecessary(const Notification<Object>& notification, Relationships::ReferenceId referencePath)
      {     
         // clear all instances of the reference id in this collection
         for (auto& resource : referencePath.collection)
         {
            for (jude_index_t arrayIndex = 0; arrayIndex < resource.CountField(referencePath.fieldIndex); arrayIndex++)
            {
               if (resource.GetFieldAsNumber<jude_id_t>(referencePath.fieldIndex, arrayIndex) == notification->Id())
               {
                  resource.Clear(referencePath.fieldIndex, arrayIndex);
                  arrayIndex--;
               }
            }
         }
      }

   }

   void Relationships::DeleteTogether(CollectionBase& collection1, CollectionBase& collection2)
   {
      m_subscribers.push_back(collection1.OnObjectDeleted([&](const Notification<Object>& notification) { 
         // If deleted in collection1, attempt to delete in the collection2
         collection2.Delete(notification->Id());
      }, "", NotifyQueue::Immediate));

      m_subscribers.push_back(collection2.OnObjectDeleted([&](const Notification<Object>& notification) {
         // If deleted in collection2, attempt to delete in the collection1
         collection1.Delete(notification->Id());
      }, "", NotifyQueue::Immediate));
   }

   void Relationships::CascadeDelete(jude::CollectionBase& from, ReferenceId to)
   {
      m_subscribers.push_back(
         from.OnObjectDeleted(
            [=](const Notification<Object>& notification) { 
               detail::CheckForDeletion(notification, to);
            },
            "",
            NotifyQueue::Immediate)
         );
   }

   void Relationships::EnforceReference(ReferenceId from, jude::CollectionBase& to, bool allowMultipleReferences)
   {
      // if the field in "from" is changed, ensure it exists in the "to" collection
      m_subscribers.push_back(
         from.collection.ValidateWith( 
            [from, allowMultipleReferences, &to](Notification<Object>& resource)
            { 
               return detail::ValidateReference(resource, from, to, allowMultipleReferences);
            })
      );

      // If a resource is removed in "to" collection, scrub field in "from" path
      m_subscribers.push_back(
         to.OnObjectDeleted(
            [from](const Notification<Object>& notification) { 
               detail::ClearReferenceFieldIfNecessary(notification, from);
            }, 
            "",
            NotifyQueue::Immediate)
      );
   }  

   void Relationships::EnforceReference(ReferenceId from, jude::CollectionBase& to)
   {
      EnforceReference(from, to, false);
   }

   void Relationships::EnforceReference_AllowDuplicates(ReferenceId from, jude::CollectionBase& to)
   {
      EnforceReference(from, to, true);
   }

}