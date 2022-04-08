#include "../core/test_base.h"
#include "autogen/alltypes_test/CustomIdObject.h"

using namespace jude;

class CustomIdTests : public JudeTestBase
{
};

TEST_F(CustomIdTests, custom_ids)
{
   auto object = jude::CustomIdObject::New();

   ASSERT_FALSE(object.IsIdAssigned());
   ASSERT_FALSE(object.Has(CustomIdObject::Index::id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::Id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::ID));

   object.Set_Id(2);

   ASSERT_FALSE(object.IsIdAssigned());
   ASSERT_FALSE(object.Has(CustomIdObject::Index::id));
   ASSERT_TRUE(object.Has(CustomIdObject::Index::Id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::ID));

   object.Set_ID(3);

   ASSERT_FALSE(object.IsIdAssigned());
   ASSERT_FALSE(object.Has(CustomIdObject::Index::id));
   ASSERT_TRUE(object.Has(CustomIdObject::Index::Id));
   ASSERT_TRUE(object.Has(CustomIdObject::Index::ID));

   object.AssignId(1);

   ASSERT_TRUE(object.IsIdAssigned());
   ASSERT_TRUE(object.Has(CustomIdObject::Index::id));
   ASSERT_TRUE(object.Has(CustomIdObject::Index::Id));
   ASSERT_TRUE(object.Has(CustomIdObject::Index::ID));

   ASSERT_EQ(object.Id(), 1);
   ASSERT_EQ(object.Get_Id(), 2);
   ASSERT_EQ(object.Get_ID(), 3);

   object.Clear_ID();

   ASSERT_TRUE(object.IsIdAssigned());
   ASSERT_TRUE(object.Has(CustomIdObject::Index::id));
   ASSERT_TRUE(object.Has(CustomIdObject::Index::Id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::ID));

   object.Clear_Id();

   ASSERT_TRUE(object.IsIdAssigned());
   ASSERT_TRUE(object.Has(CustomIdObject::Index::id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::Id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::ID));

   object.Clear();

   ASSERT_FALSE(object.IsIdAssigned());
   ASSERT_FALSE(object.Has(CustomIdObject::Index::id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::Id));
   ASSERT_FALSE(object.Has(CustomIdObject::Index::ID));
}
