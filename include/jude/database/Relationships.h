#pragma once

#include <jude/database/Database.h>
#include <jude/database/Collection.h>

namespace jude { 
   
   class Relationships
   {
      std::vector<SubscriptionHandle> m_subscribers;

   public:
      struct ReferenceId // a "path" to a reference ID in a collection
      {
         ReferenceId(jude::CollectionBase& c, jude_index_t i = JUDE_ID_FIELD_INDEX) : collection(c), fieldIndex(i) {}
         jude::CollectionBase& collection;
         jude_index_t fieldIndex;
      };

      ~Relationships()
      {
         ClearAll();
      };

      void ClearAll()
      {
         for (auto& handle : m_subscribers)
         {
            handle.Unsubscribe();
         }
      }

      // Use this relationship to keep common id's from two collections in sync
      // Deleting id "X" from one collection deletes id "X" from the other (if it exists)
      void DeleteTogether(jude::CollectionBase& twin1, jude::CollectionBase& twin2);

      // Whenever the reference object in "from" is removed, we shall delete the entity in "to" that referred to it.
      void CascadeDelete(jude::CollectionBase& from, ReferenceId to);

      // Use this relationship enforce a many to one relationship with these two features:
      // 1. The referenceCollectonReferenceId will be validated against changes to ensure each element points to 
      //    a valid entry in the target collection
      // 2. When an entry in the target collection is deleted, the reference paths are scanned and 
      //    scrubbed of any references to the deleted resource.
      // 3. If allowDuplicatesInReference is false then we must ensure that no two elements in 
      //    the given reference paths refer to the same element in the target collection
      void EnforceReference(ReferenceId from, jude::CollectionBase& to);
      void EnforceReference_AllowDuplicates(ReferenceId from, jude::CollectionBase& to);

   private:
      void EnforceReference(ReferenceId from, jude::CollectionBase& to, bool allowMultipleReferences);

   };
}