#include <gtest/gtest.h>
#include <inttypes.h>

#include <jude/restapi/jude_browser.h>
#include <jude/database/Collection.h>
#include <jude/database/Relationships.h>

#include "../core/test_base.h"

#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

using namespace jude;

class Relationship_OneToMany : public JudeTestBase
{
public:
   Collection<SubMessage> m_targetCollection;
   Collection<AllOptionalTypes> m_referencingCollection;
   Collection<AllRepeatedTypes> m_referencingCollectionWithRepeats;
   Relationships m_relationships;

   Relationship_OneToMany()
      : m_targetCollection("Target", 1024, jude_user_Public)
      , m_referencingCollection("Referencing", 1024, jude_user_Public)
      , m_referencingCollectionWithRepeats("Referencing", 1024, jude_user_Public)
   {
   }

   void SomeMoreCollectionData()
   {
      m_targetCollection.Post();
      m_referencingCollection.Post();
   }

   void ObjectInTargetCollectionReferencedFromOtherCollection(jude_id_t targetId, jude_id_t referecingId)
   {
      m_targetCollection.Post(targetId);

      // use the u64 field to reference the targetCollection
      m_referencingCollection.Post(referecingId)->Set_uint64_type(targetId);
   }

   void ObjectInTargetCollectionReferencedFromOtherCollection(std::initializer_list<jude_id_t> targetIds, jude_id_t referecingId)
   {
      for (auto id : targetIds)
      {
         m_targetCollection.Post(id);
      }

      auto resource = m_referencingCollectionWithRepeats.Post(referecingId);;
      for (auto id : targetIds)
      {
         resource->Get_uint64_types().Add(id);
      }
      
   }

};

TEST_F(Relationship_OneToMany, clears_field_of_reference_on_deletion_of_object)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   ObjectInTargetCollectionReferencedFromOtherCollection(targetId, referecingId);
   SomeMoreCollectionData();

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_TRUE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(4, m_targetCollection.count());
   ASSERT_TRUE(m_referencingCollection.WriteLock(referecingId)->Has_uint64_type());

   m_relationships.EnforceReference({ m_referencingCollection, AllOptionalTypes::Index::uint64_type }, m_targetCollection);

   m_targetCollection.Delete(targetId);

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(3, m_targetCollection.count());
   // uint64 type has been cleared due to resource deletion
   ASSERT_FALSE(m_referencingCollection.WriteLock(referecingId)->Has_uint64_type());
}

TEST_F(Relationship_OneToMany, clears_array_entry_of_reference_on_deletion_of_object)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   ObjectInTargetCollectionReferencedFromOtherCollection({ 1, 2, 3, targetId, 5 }, referecingId);

   ASSERT_TRUE(m_referencingCollectionWithRepeats.ContainsId(referecingId));
   ASSERT_TRUE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(1, m_referencingCollectionWithRepeats.count());
   ASSERT_EQ(5, m_targetCollection.count());
   ASSERT_EQ(5, m_referencingCollectionWithRepeats.WriteLock(referecingId)->Get_uint64_types().count());
   ASSERT_STREQ(m_referencingCollectionWithRepeats.WriteLock(referecingId)->ToJSON().c_str(), 
                R"({"id":5678,"uint64_type":[1,2,3,1234,5]})");

   m_relationships.EnforceReference({ m_referencingCollectionWithRepeats, AllOptionalTypes::Index::uint64_type }, m_targetCollection);

   m_targetCollection.Delete(targetId);

   ASSERT_TRUE(m_referencingCollectionWithRepeats.ContainsId(referecingId));
   ASSERT_EQ(1, m_referencingCollectionWithRepeats.count());
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_targetCollection.count());
   // id entry has been cleared due to resource deletion
   ASSERT_EQ(4, m_referencingCollectionWithRepeats.WriteLock(referecingId)->Get_uint64_types().count());
   ASSERT_STREQ(m_referencingCollectionWithRepeats.WriteLock(referecingId)->ToJSON().c_str(),
               R"({"id":5678,"uint64_type":[1,2,3,5]})");
}

TEST_F(Relationship_OneToMany, validates_array_change_and_ensures_referred_id_exists)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   m_relationships.EnforceReference({ m_referencingCollectionWithRepeats, AllOptionalTypes::Index::uint64_type }, m_targetCollection);

   ObjectInTargetCollectionReferencedFromOtherCollection({ 1, 2, 3, targetId, 5 }, referecingId);

   ASSERT_TRUE(m_referencingCollectionWithRepeats.ContainsId(referecingId));
   ASSERT_TRUE(m_targetCollection.ContainsId(targetId));

   {
      auto lock = m_referencingCollectionWithRepeats.TransactionLock(referecingId);
      lock->Get_uint64_types().Add(789); // should fail to validate because 789 does not exist
      auto result = lock.Commit();
      ASSERT_FALSE(result.IsOK());
      ASSERT_STREQ(result.GetDetails().c_str(), "'Referencing/5678/uint64_type' refers to id 789 which is not in collection 'Target'");
   }

   // make 789 exist
   m_targetCollection.Post(789);

   {
      auto lock = m_referencingCollectionWithRepeats.TransactionLock(referecingId);
      lock->Get_uint64_types().Add(789); // should fail to validate because 789 does not exist
      auto result = lock.Commit();
      ASSERT_TRUE(result.IsOK());
   }

}

TEST_F(Relationship_OneToMany, collection_dont_sync_without_relationship)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   ObjectInTargetCollectionReferencedFromOtherCollection(targetId, referecingId);
   SomeMoreCollectionData();

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_TRUE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(4, m_targetCollection.count());

   m_targetCollection.Delete(targetId);

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(3, m_targetCollection.count());
}

TEST_F(Relationship_OneToMany, collection_dont_sync_if_relationship_falls_out_of_scope)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   ObjectInTargetCollectionReferencedFromOtherCollection(targetId, referecingId);
   SomeMoreCollectionData();

   m_relationships.EnforceReference({ m_referencingCollection, AllOptionalTypes::Index::uint64_type }, m_targetCollection);

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_TRUE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(4, m_targetCollection.count());

   m_relationships.ClearAll();

   m_targetCollection.Delete(targetId);

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(3, m_targetCollection.count());
}

TEST_F(Relationship_OneToMany, relationship_does_not_affect_unique_object_ids)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   SomeMoreCollectionData();

   // create but this time do not include the reference - just two plain resources
   m_referencingCollection.Post(referecingId);
   m_targetCollection.Post(targetId);

   m_relationships.EnforceReference({ m_referencingCollection, AllOptionalTypes::Index::uint64_type }, m_targetCollection);

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_TRUE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(4, m_targetCollection.count());

   m_targetCollection.Delete(targetId);

   ASSERT_TRUE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(4, m_referencingCollection.count());
   ASSERT_EQ(3, m_targetCollection.count());

   m_referencingCollection.Delete(referecingId);

   ASSERT_FALSE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(3, m_referencingCollection.count());
   ASSERT_EQ(3, m_targetCollection.count());
}
