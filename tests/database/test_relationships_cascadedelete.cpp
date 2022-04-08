#include <gtest/gtest.h>
#include <inttypes.h>

#include <jude/restapi/jude_browser.h>
#include <jude/database/Collection.h>
#include <jude/database/Relationships.h>

#include "../core/test_base.h"

#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

using namespace jude;

class Relationship_CascadeDelete : public JudeTestBase
{
public:
   Collection<SubMessage> m_targetCollection;
   Collection<AllOptionalTypes> m_referencingCollection;
   Relationships m_relationships;

   Relationship_CascadeDelete()
      : m_targetCollection("Target", 1024, jude_user_Public)
      , m_referencingCollection("Referencing", 1024, jude_user_Public)
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
};

TEST_F(Relationship_CascadeDelete, cascade_delete_clears_field_of_reference_on_deletion_of_object)
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

   m_relationships.CascadeDelete(m_targetCollection, { m_referencingCollection, AllOptionalTypes::Index::uint64_type } );

   m_targetCollection.Delete(targetId);

   ASSERT_FALSE(m_referencingCollection.ContainsId(referecingId));
   ASSERT_FALSE(m_targetCollection.ContainsId(targetId));
   ASSERT_EQ(3, m_referencingCollection.count());
   ASSERT_EQ(3, m_targetCollection.count());
}

TEST_F(Relationship_CascadeDelete, collection_dont_sync_without_relationship)
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

TEST_F(Relationship_CascadeDelete, collection_dont_sync_if_relationship_falls_out_of_scope)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   ObjectInTargetCollectionReferencedFromOtherCollection(targetId, referecingId);
   SomeMoreCollectionData();

   m_relationships.CascadeDelete(m_targetCollection, { m_referencingCollection, AllOptionalTypes::Index::uint64_type } );

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

TEST_F(Relationship_CascadeDelete, relationship_does_not_affect_unique_object_ids)
{
   jude_id_t targetId = 1234;
   jude_id_t referecingId = 5678;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   SomeMoreCollectionData();

   // create but this time do not include the reference - just two plain resources
   m_referencingCollection.Post(referecingId);
   m_targetCollection.Post(targetId);

   m_relationships.CascadeDelete(m_targetCollection, { m_referencingCollection, AllOptionalTypes::Index::uint64_type } );

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
