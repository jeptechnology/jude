#include <gtest/gtest.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Collection.h"
#include <jude/database/Relationships.h>

using namespace jude;

class Relationship_DeleteTogether : public JudeTestBase
{
public:
   Collection<SubMessage> m_collection1;
   Collection<SubMessage> m_collection2;
   Relationships m_relationships;

   Relationship_DeleteTogether()
      : m_collection1("MyCollection1", 1024, jude_user_Public)
      , m_collection2("MyCollection2", 1024, jude_user_Public)
   {
   }

   void SomeMoreCollectionData()
   {
      m_collection1.Post();
      m_collection2.Post();
   }

   void TwoCollectionsContainMatchingObject(jude_id_t duplicateId)
   {
      m_collection1.Post(duplicateId);
      m_collection2.Post(duplicateId);
   }
};

TEST_F(Relationship_DeleteTogether, collection_sync_when_deleted_on_collection1)
{
   jude_id_t duplicateId = 12345;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   TwoCollectionsContainMatchingObject(duplicateId);
   SomeMoreCollectionData();

   ASSERT_TRUE(m_collection1.ContainsId(duplicateId));
   ASSERT_TRUE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(4, m_collection2.count());

   m_relationships.DeleteTogether(m_collection1, m_collection2);
   m_collection1.Delete(duplicateId);

   ASSERT_FALSE(m_collection1.ContainsId(duplicateId));
   ASSERT_FALSE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(3, m_collection1.count());
   ASSERT_EQ(3, m_collection2.count());
}

TEST_F(Relationship_DeleteTogether, collection_sync_when_deleted_on_collection2)
{
   jude_id_t duplicateId = 12345;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   TwoCollectionsContainMatchingObject(duplicateId);
   SomeMoreCollectionData();

   ASSERT_TRUE(m_collection1.ContainsId(duplicateId));
   ASSERT_TRUE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(4, m_collection2.count());

   m_relationships.DeleteTogether(m_collection1, m_collection2);
   m_collection2.Delete(duplicateId);

   ASSERT_FALSE(m_collection1.ContainsId(duplicateId));
   ASSERT_FALSE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(3, m_collection1.count());
   ASSERT_EQ(3, m_collection2.count());
}

TEST_F(Relationship_DeleteTogether, collection_dont_sync_without_relationship)
{
   jude_id_t duplicateId = 12345;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   TwoCollectionsContainMatchingObject(duplicateId);
   SomeMoreCollectionData();

   ASSERT_TRUE(m_collection1.ContainsId(duplicateId));
   ASSERT_TRUE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(4, m_collection2.count());

   m_collection2.Delete(duplicateId);

   ASSERT_TRUE(m_collection1.ContainsId(duplicateId));
   ASSERT_FALSE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(3, m_collection2.count());
}

TEST_F(Relationship_DeleteTogether, collection_dont_sync_if_relationship_falls_out_of_scope)
{
   jude_id_t duplicateId = 12345;

   SomeMoreCollectionData();
   SomeMoreCollectionData();
   TwoCollectionsContainMatchingObject(duplicateId);
   SomeMoreCollectionData();

   m_relationships.DeleteTogether(m_collection1, m_collection2);

   ASSERT_TRUE(m_collection1.ContainsId(duplicateId));
   ASSERT_TRUE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(4, m_collection2.count());

   m_relationships.ClearAll();

   m_collection2.Delete(duplicateId);

   ASSERT_TRUE(m_collection1.ContainsId(duplicateId));
   ASSERT_FALSE(m_collection2.ContainsId(duplicateId));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(3, m_collection2.count());
}

TEST_F(Relationship_DeleteTogether, relationship_does_not_affect_unique_object_ids)
{
   m_relationships.DeleteTogether(m_collection1, m_collection2);
   
   SomeMoreCollectionData();

   m_collection1.Post(1234);
   m_collection2.Post(5678);

   SomeMoreCollectionData();
   SomeMoreCollectionData();

   ASSERT_TRUE(m_collection1.ContainsId(1234));
   ASSERT_TRUE(m_collection2.ContainsId(5678));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(4, m_collection2.count());

   m_collection2.Delete(5678);

   ASSERT_TRUE(m_collection1.ContainsId(1234));
   ASSERT_FALSE(m_collection2.ContainsId(5678));
   ASSERT_EQ(4, m_collection1.count());
   ASSERT_EQ(3, m_collection2.count());

   m_collection1.Delete(1234);

   ASSERT_FALSE(m_collection1.ContainsId(1234));
   ASSERT_FALSE(m_collection2.ContainsId(5678));
   ASSERT_EQ(3, m_collection1.count());
   ASSERT_EQ(3, m_collection2.count());
}
