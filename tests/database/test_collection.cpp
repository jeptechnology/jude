#include <gtest/gtest.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Collection.h"

using namespace jude;

class CollectionTests : public JudeTestBase
{
   static constexpr size_t CollectionCapacity = 50;

public:
   Collection<SubMessage> m_collection;
   size_t m_notificationCount;
   size_t m_notificationOfNewObjects;
   size_t m_notificationOfDeletedObjects;

   std::string m_validationError;
   size_t m_validationCount;

   CollectionTests()
      : m_collection("MyCollection", CollectionCapacity, jude_user_Root)
      , m_notificationCount(0)
      , m_notificationOfNewObjects(0)
      , m_notificationOfDeletedObjects(0)
      , m_validationError("")
      , m_validationCount(0)

   {
   }

   void TestCallback(const Notification<SubMessage>& notification)
   {
      m_notificationCount++;

      if (notification.IsNew())
      {
         m_notificationOfNewObjects++;
      }
      if (notification.IsDeleted())
      {
         m_notificationOfDeletedObjects++;
      }

      if (notification)
      {
         // if its not a deleted object then it is a root object as it lives inside our collection as a top level resource
         ASSERT_FALSE(notification->Parent().IsOK());
      }
   }

   void TestCallback(const Notification<>& notification)
   {
      m_notificationCount++;

      if (notification.IsNew())
      {
         m_notificationOfNewObjects++;
      }
      if (notification.IsDeleted())
      {
         m_notificationOfDeletedObjects++;
      }

      if (notification)
      {
         // if its not a deleted object then it is a root object as it lives inside our collection as a top level resource
         ASSERT_FALSE(notification->Parent().IsOK());
      }
   }

   ValidationResult TestValidator(Notification<SubMessage>&)
   {
      m_validationCount++;

      if (m_validationError.length() > 0)
      {
         return m_validationError;
      }
      return true;
   }

   void GenericTestCallback(const Notification<Object>& notification)
   {
      m_notificationCount++;
      if (notification)
      {
         // if its not a deleted object then it is a root object as it lives inside our collection as a top level resource
         ASSERT_FALSE(notification->Parent().IsOK());
      }
   }

   bool AddObjectWithId(jude_id_t id, const char* substuff1 = nullptr, int32_t substuff2 = 0, bool* substuff3 = nullptr)
   {
      auto tmp = SubMessage::New();
      if (substuff1) tmp.Set_substuff1(substuff1);
      if (substuff2) tmp.Set_substuff2(substuff2);
      if (substuff3) tmp.Set_substuff3(substuff3);

      if (uuid_counter <= id)
      {
         uuid_counter = id;
      }

      auto post = m_collection.Post(id);
      post->Put(tmp);
      return post.Commit().IsOK();
   }

   bool AddObjectWithId(jude_id_t id, const char* substuff1, int32_t substuff2, bool substuff3)
   {
      return AddObjectWithId(id, substuff1, substuff2, &substuff3);
   }

   void AddObjectsWithIds(std::initializer_list<jude_id_t> idList)
   {
      for (auto id : idList)
      {
         ASSERT_TRUE(AddObjectWithId(id));
      }
   }

   void VerifyRangeLoopHas(std::initializer_list<jude_id_t> idList, const char* msg = nullptr)
   {
      size_t expectedCount = idList.size();
      size_t count = 0;
      std::vector<jude_id_t> ids(idList);

      for (const auto& sub : m_collection)
      {
         ASSERT_EQ(ids[count], sub.Id()) << msg;
         count++;
      }

      ASSERT_EQ(expectedCount, count) << msg;
   }

   void CheckPublishOnAddAndRemove(bool subscribed_to_id)
   {
      auto expectedNotificationCount = m_notificationCount;
      auto expectedAddCount = m_notificationOfNewObjects;
      auto expectedDeletedCount = m_notificationOfDeletedObjects;

      ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "adding should not auto-publish";

      auto id = m_collection.Post()->Id();

      if (subscribed_to_id)
      {
         expectedNotificationCount++;
         expectedAddCount++;
      }
      ASSERT_EQ(expectedNotificationCount, m_notificationCount) << (subscribed_to_id ? "subscribed to id, should have notified on Add" : "not subscribed to 'id' did not expect notify on Add");
      ASSERT_EQ(expectedAddCount, m_notificationOfNewObjects) << (subscribed_to_id ? "subscribed to id, should have notified a *NEW* resource" : "not subscribed to 'id' did not expect notify on Add");
      ASSERT_EQ(expectedDeletedCount, m_notificationOfDeletedObjects) << (subscribed_to_id ? "subscribed to id, should have notified a *NEW* resource" : "not subscribed to 'id' did not expect notify on Add");

      ASSERT_REST_OK(m_collection.Delete(id));

      if (subscribed_to_id)
      {
         expectedNotificationCount++;
         expectedDeletedCount++;
      }
      ASSERT_EQ(expectedNotificationCount, m_notificationCount) << (subscribed_to_id ? "subscribed to id, should have notified on Remove" : "not subscribed to 'id' did not expect notify on Remove");
      ASSERT_EQ(expectedAddCount, m_notificationOfNewObjects) << (subscribed_to_id ? "subscribed to id, unexpected Add notify count after delete" : "not subscribed to 'id', unexpected Add notify count after delete");
      ASSERT_EQ(expectedDeletedCount, m_notificationOfDeletedObjects) << (subscribed_to_id ? "subscribed to id, should have notified a *REMOVED* resource" : "not subscribed to 'id', unexpected Delete notify count after delete");
   }

   void Check_No_Publish_For_Field(const char* fieldName, const char* value1, const char* value2)
   {
      Check_Publish_For_Field(fieldName, value1, value2, false);
   }

   void Check_No_Publish_For_Field(jude_id_t requestedId, const char* fieldName, const char* value1, const char* value2)
   {
      Check_Publish_For_Field(requestedId, fieldName, value1, value2, false);
   }

   void Check_Publish_For_Field(const char* fieldName, const char* value1, const char* value2, bool expectPublish = true)
   {
      Check_Publish_For_Field(JUDE_AUTO_ID, fieldName, value1, value2, expectPublish);
   }

   void Check_Publish_For_Field(jude_id_t requestedId, const char* fieldName, const char* value1, const char* value2, bool expectPublish = true)
   {
      char fullpath[32];

      auto id = m_collection.Post(requestedId)->Id();
      snprintf(fullpath, sizeof(fullpath), "%" PRIjudeID "/%s", id, fieldName);

      auto expectedNotificationCount = m_notificationCount;

      ASSERT_REST_OK(m_collection.RestPatchString(fullpath, value1));

      if (expectPublish) expectedNotificationCount++;
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
         << "setting " << fieldName << " should " << (expectPublish ? "" : "not ") << "cause notification on publish";

      ASSERT_REST_OK(m_collection.RestPatchString(fullpath, value1));
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
         << "setting " << fieldName << " to same value should not notify";

      ASSERT_REST_OK(m_collection.RestPatchString(fullpath, value2));
      if (expectPublish) expectedNotificationCount++;
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
         << "setting " << fieldName << " to different value should " << (expectPublish ? "notify" : "not notify as no subscription");

      ASSERT_REST_OK(m_collection.Delete(id));
   }

   void Check_Const_Functions_Do_Not_Publish()
   {
      auto expectedCount = m_notificationCount;
      const auto& m_constCollection = m_collection;

      m_constCollection.count();
      m_constCollection.cbegin();
      m_constCollection.cend();
      m_constCollection.ReadLock(0);
      m_constCollection.ReadLockNext(0);
      m_constCollection.GetAccessLevel(jude::CRUD::READ);
      m_constCollection.GetName();
      m_constCollection.ToJSON();
      ASSERT_EQ(expectedCount, m_notificationCount) << "const functions should not create new notifications";
   }
};

TEST_F(CollectionTests, collection_is_empty_on_init)
{
   ASSERT_EQ(0, m_collection.count());
   ASSERT_FALSE(m_collection.WriteLockNext(1).IsOK());
   ASSERT_FALSE(m_collection.Delete(1));
   ASSERT_STREQ(R"({})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, collection_add_increases_count)
{
   ASSERT_TRUE(AddObjectWithId(1));
   ASSERT_EQ(1, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1}})", m_collection.ToJSON().c_str());

   ASSERT_TRUE(AddObjectWithId(2));
   ASSERT_EQ(2, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1},"2":{"id":2}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, collection_can_find)
{
   ASSERT_FALSE(m_collection(1).IsOK());
   ASSERT_FALSE(m_collection(2).IsOK());
   ASSERT_FALSE(m_collection(3).IsOK());

   ASSERT_FALSE(m_collection.WriteLockNext(1).IsOK());
   ASSERT_FALSE(m_collection.WriteLockNext(2).IsOK());
   ASSERT_FALSE(m_collection.WriteLockNext(3).IsOK());

   ASSERT_TRUE(AddObjectWithId(2));

   ASSERT_FALSE(m_collection(1).IsOK());
   ASSERT_TRUE (m_collection(2).IsOK());
   ASSERT_FALSE(m_collection(3).IsOK());

   ASSERT_TRUE (m_collection.WriteLockNext(1).IsOK());
   ASSERT_FALSE(m_collection.WriteLockNext(2).IsOK());
   ASSERT_FALSE(m_collection.WriteLockNext(3).IsOK());
}

TEST_F(CollectionTests, transaction_copies_data_on_access)
{
   ASSERT_TRUE(AddObjectWithId(1));

   auto result = m_collection.TransactionLock(1);
   ASSERT_TRUE(result.IsOK());
   result->Set_substuff1("World");

   // Note that "World" does not appear in the collection yet...
   ASSERT_STREQ(R"({"1":{"id":1}})", m_collection.ToJSON().c_str());

   // When I change the resource value to "Hello" and patch it back to the collection...
   result->Set_substuff1("Hello");
   auto patchResult = result.Commit();
   ASSERT_TRUE(patchResult.IsOK());
   // Then I see the "Hello" in the collection...
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, write_transaction_handles_attempted_id_change_gracefully)
{
   ASSERT_TRUE(AddObjectWithId(1));


   if (auto transaction = m_collection.TransactionLock(1))
   {
      ASSERT_TRUE(transaction.IsOK());
      transaction->Set_substuff1("World");

      // OOOH - sneaky! - we are changing the id in the middle of a transaction... that is not allowed
      transaction->AssignId(999);

      auto result = transaction.Commit();
      ASSERT_TRUE(result) << result.GetDetails();
   }
   
   // We still get the commit but the id remains unchanged
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"World"}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, postlock_adds_to_collection_only_on_commit)
{
   ASSERT_FALSE(m_collection.ContainsId(1));

   auto result = m_collection.Post(1);
   ASSERT_TRUE(result.IsOK());
   result->Set_substuff1("Hello");

   // Note that new resource does not appear in the collection yet...
   ASSERT_STREQ(R"({})", m_collection.ToJSON().c_str());

   // When I commit the resource 
   auto postResult = result.Commit();
   ASSERT_TRUE(postResult.IsOK());
   // Then I see the "Hello" in the collection...
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, postlock_does_not_add_to_collection_on_abort)
{
   ASSERT_FALSE(m_collection.ContainsId(1));

   {
      auto result = m_collection.Post(1);
      ASSERT_TRUE(result.IsOK());
      result->Set_substuff1("Hello");

      // Note that new resource does not appear in the collection yet...
      ASSERT_STREQ(R"({})", m_collection.ToJSON().c_str());

      // When I commit the resource 
      result.Abort();
   }

   // Then I do not see the new resource in the collection...
   ASSERT_STREQ(R"({})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, collection_Rest_GET_aka_ToJSON)
{
   // Note that we are testing the Rest GET method with the ToJSON method as that is a simple wrapper for the GET
   AddObjectWithId(1, "Hello");
   AddObjectWithId(2, nullptr, 1234);
   AddObjectWithId(3, nullptr, 0, true);

   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"2":{"id":2,"substuff2":1234},"3":{"id":3,"substuff3":true}})", m_collection.ToJSON().c_str());

   // Paths into resource 1
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("1").c_str());
   ASSERT_STREQ(R"("Hello")", m_collection.ToJSON("1/substuff1").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("1/substuff2").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("1/substuff3").c_str());

   // Paths into resource 2
   ASSERT_STREQ(R"({"id":2,"substuff2":1234})", m_collection.ToJSON("2").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("2/substuff1").c_str());
   ASSERT_STREQ(R"(1234)", m_collection.ToJSON("2/substuff2").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("2/substuff3").c_str());

   // Paths into resource 3
   ASSERT_STREQ(R"({"id":3,"substuff3":true})", m_collection.ToJSON("3").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("3/substuff1").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("3/substuff2").c_str());
   ASSERT_STREQ(R"(true)", m_collection.ToJSON("3/substuff3").c_str());

   // fullpath to non-existent resource in this collection...
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_collection.ToJSON("5").c_str());
}

TEST_F(CollectionTests, collection_Rest_POST)
{
   // Create one resource directly...
   AddObjectWithId(jude_generate_uuid(), "Hello");
   ASSERT_EQ(1, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"}})", m_collection.ToJSON().c_str());

   // Create a resource via a Restful POST...
   ASSERT_REST_OK(m_collection.RestPostString("/", R"({"substuff2":1234})"));
   ASSERT_EQ(2, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"2":{"id":2,"substuff2":1234}})", m_collection.ToJSON().c_str());

   // Create with id given but should be ignored...
   ASSERT_REST_OK(m_collection.RestPostString("/", R"({"id":123,"substuff3":true})"));
   ASSERT_EQ(3, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"2":{"id":2,"substuff2":1234},"3":{"id":3,"substuff3":true}})", m_collection.ToJSON().c_str());

   // Failures should be captured...
   auto result = m_collection.RestPostString("/", R"({"id":123,"substuff3":wrong})");
   ASSERT_EQ(result.GetCode(), jude_rest_Bad_Request);
   ASSERT_EQ(result.GetDetails(), "substuff3: Expected true, false or null");
   ASSERT_EQ(3, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"2":{"id":2,"substuff2":1234},"3":{"id":3,"substuff3":true}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, collection_Rest_PATCH)
{
   // Create one resource directly...
   AddObjectWithId(1, "Hello");
   ASSERT_EQ(1, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"}})", m_collection.ToJSON().c_str());

   ASSERT_FALSE(m_collection.RestPatchString("/", R"({"substuff2":1234})").IsOK()) << "Should not be able to patch root fullpath in collection";
   ASSERT_FALSE(m_collection.RestPatchString("/2", R"({"substuff2":1234})").IsOK()) << "Should not be able to patch non existent resource in collection";

   ASSERT_TRUE(m_collection.RestPatchString("/1", R"({"substuff2":1234})").IsOK());

   ASSERT_EQ(1, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello","substuff2":1234}})", m_collection.ToJSON().c_str());

   ASSERT_TRUE(m_collection.RestPatchString("1", R"({"substuff2":5678,"substuff1":"World"})").IsOK());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"World","substuff2":5678}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, collection_Rest_PUT)
{
   // Create one resource directly...
   AddObjectWithId(1, "Hello");
   ASSERT_EQ(1, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"}})", m_collection.ToJSON().c_str());

   ASSERT_FALSE(m_collection.RestPutString("/", R"({"substuff2":1234})").IsOK()) << "Should not be able to put root fullpath in collection";
   ASSERT_FALSE(m_collection.RestPutString("/2", R"({"substuff2":1234})").IsOK()) << "Should not be able to put non existent resource in collection";

   ASSERT_TRUE(m_collection.RestPutString("/1", R"({"substuff2":1234})").IsOK());

   ASSERT_EQ(1, m_collection.count());
   // Note: "Hello" should be removed due to PUT replacing everything given (apart from id!
   ASSERT_STREQ(R"({"1":{"id":1,"substuff2":1234}})", m_collection.ToJSON().c_str());

   ASSERT_TRUE(m_collection.RestPutString("1", R"({"substuff2":5678,"substuff1":"World"})").IsOK());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"World","substuff2":5678}})", m_collection.ToJSON().c_str());
}

TEST_F(CollectionTests, collection_Rest_DELETE)
{
   AddObjectWithId(1, "Hello");
   AddObjectWithId(2, nullptr, 1234);
   AddObjectWithId(3, nullptr, 0, true);

   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"2":{"id":2,"substuff2":1234},"3":{"id":3,"substuff3":true}})", m_collection.ToJSON().c_str());
   ASSERT_EQ(3, m_collection.count());

   ASSERT_FALSE(m_collection.RestDelete("/").IsOK()) << "Should not be able to delete the entire collection";
   ASSERT_FALSE(m_collection.RestDelete("/5").IsOK()) << "Should not be able to delete non existent resource in collection";
   ASSERT_FALSE(m_collection.RestDelete("5").IsOK()) << "Should not be able to delete non existent resource in collection";

   ASSERT_TRUE(m_collection.RestDelete("/2").IsOK());
   ASSERT_EQ(2, m_collection.count());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"3":{"id":3,"substuff3":true}})", m_collection.ToJSON().c_str());
   ASSERT_FALSE(m_collection.RestDelete("/2").IsOK());

   ASSERT_TRUE(m_collection.RestDelete("3").IsOK());
   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"}})", m_collection.ToJSON().c_str());
   ASSERT_EQ(1, m_collection.count());

   ASSERT_TRUE(m_collection.RestDelete("1/substuff1").IsOK());
   ASSERT_STREQ(R"({"1":{"id":1}})", m_collection.ToJSON().c_str());
   ASSERT_EQ(1, m_collection.count());
}

TEST_F(CollectionTests, patch_only_updates_deltas)
{
   AddObjectWithId(1);

   auto lock = m_collection.TransactionLock(1);
   ASSERT_TRUE(lock.IsOK());
   lock->Set_substuff1("World");
   lock->Set_substuff2(1234);

   ASSERT_STREQ(R"({"1":{"id":1}})", m_collection.ToJSON().c_str());

   auto patchResult = lock.Commit();
   ASSERT_TRUE(patchResult.IsOK());

   ASSERT_STREQ(R"({"1":{"id":1,"substuff1":"World","substuff2":1234}})", m_collection.ToJSON().c_str());
}


TEST_F(CollectionTests, collection_remove)
{
   AddObjectWithId(1);
   AddObjectWithId(2);
   AddObjectWithId(3);

   ASSERT_STREQ(R"({"1":{"id":1},"2":{"id":2},"3":{"id":3}})", m_collection.ToJSON().c_str());

   m_collection.Delete(5);
   ASSERT_STREQ(R"({"1":{"id":1},"2":{"id":2},"3":{"id":3}})", m_collection.ToJSON().c_str());
   ASSERT_EQ(3, m_collection.count());

   m_collection.Delete(2);
   ASSERT_STREQ(R"({"1":{"id":1},"3":{"id":3}})", m_collection.ToJSON().c_str());
   ASSERT_EQ(2, m_collection.count());

   m_collection.Delete(3);
   ASSERT_STREQ(R"({"1":{"id":1}})", m_collection.ToJSON().c_str());
   ASSERT_EQ(1, m_collection.count());

   m_collection.Delete(1);
   ASSERT_STREQ(R"({})", m_collection.ToJSON().c_str());
   ASSERT_EQ(0, m_collection.count());
}

TEST_F(CollectionTests, subscribe_to_all)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_collection.OnChange([&](auto& info) { return TestCallback(info); });
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_Publish_For_Field("substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field("substuff2", "123", "456");
   Check_Publish_For_Field("substuff3", "false", "true");
   CheckPublishOnAddAndRemove(true);
}

TEST_F(CollectionTests, subscribe_to_multiple_fields)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_collection.OnChange(
      [&](auto& info) { return TestCallback(info); },
      {
         SubMessage::Index::substuff1,
         SubMessage::Index::substuff3
      });

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_Publish_For_Field("substuff1", R"("Hello")", R"("World")");
   Check_No_Publish_For_Field("substuff2", "123", "456");
   Check_Publish_For_Field("substuff3", "false", "true");
   CheckPublishOnAddAndRemove(false);
}

TEST_F(CollectionTests, subscribe_to_single)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_collection.OnChange(      
      [&](auto& info) { return TestCallback(info); },
      { SubMessage::Index::substuff2 });

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";
   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_No_Publish_For_Field("substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field("substuff2", "123", "456");
   Check_No_Publish_For_Field("substuff3", "false", "true");
   CheckPublishOnAddAndRemove(false);

   subscriptionHandle.Unsubscribe();
   Check_No_Publish_For_Field("substuff2", "123", "456");
}

TEST_F(CollectionTests, subscribe_to_single_path)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_collection.OnChangeToPath(
      "789/substuff2",
      [&](auto& info) { return TestCallback(info); });

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";
   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_No_Publish_For_Field(789, "substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field(789, "substuff2", "123", "456");
   Check_No_Publish_For_Field(789, "substuff3", "false", "true");
   CheckPublishOnAddAndRemove(false);

   subscriptionHandle.Unsubscribe();
   Check_No_Publish_For_Field("substuff2", "123", "456");
}

TEST_F(CollectionTests, subscribe_to_single_wildcard_path)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_collection.OnChangeToPath(
      "+/substuff2",
      [&](auto& info) { return TestCallback(info); });

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";
   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_No_Publish_For_Field("substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field("substuff2", "123", "456");
   Check_No_Publish_For_Field("substuff3", "false", "true");
   CheckPublishOnAddAndRemove(true);

   subscriptionHandle.Unsubscribe();
   Check_No_Publish_For_Field("substuff2", "123", "456");
}

TEST_F(CollectionTests, subscribe_to_add)
{
   auto subscriptionHandle = m_collection.OnObjectAdded([&](auto& info) { return GenericTestCallback(info); });
   auto expectedNotificationCount = m_notificationCount;

   auto id = m_collection.Post()->Id();
   expectedNotificationCount++;
   ASSERT_EQ(expectedNotificationCount, m_notificationCount);

   m_collection.Delete(id);
   ASSERT_EQ(expectedNotificationCount, m_notificationCount);
}

TEST_F(CollectionTests, subscribe_to_delete)
{
   auto subscriptionHandle = m_collection.OnObjectDeleted([&](auto& info) { return GenericTestCallback(info); });
   auto expectedNotificationCount = m_notificationCount;

   auto id = m_collection.Post()->Id();
   ASSERT_EQ(expectedNotificationCount, m_notificationCount);

   m_collection.Delete(id);
   expectedNotificationCount++;
   ASSERT_EQ(expectedNotificationCount, m_notificationCount);
}

TEST_F(CollectionTests, chained_subscribers_do_not_corrupt_memory)
{
   auto subscriptionHandle1 = m_collection.OnChangeToObject(      
      [&](const jude::Notification<jude::Object>& notification) {
         m_collection.WriteLock(notification->Id())->Set_substuff2(123);
      },
      { jude::SubMessage::Index::substuff1 }
   );

   auto subscriptionHandle2 = m_collection.OnChangeToObject(
      [&](const jude::Notification<jude::Object>& notification) {
         m_collection.WriteLock(notification->Id())->Set_substuff3(false);
      },
      { jude::SubMessage::Index::substuff2 }
   );

   auto subscriptionHandle3 = m_collection.OnChange(      
      [&](const Notification<SubMessage>& notification) {
         auto& readLock = *notification;
         ASSERT_EQ("Hello", readLock.Get_substuff1());
         ASSERT_EQ(123, readLock.Get_substuff2());
         ASSERT_EQ(false, readLock.Get_substuff3());
      },
      { jude::SubMessage::Index::substuff3 }
   );

   auto id = m_collection.Post()->Set_substuff1("Hello").Id();

   ASSERT_TRUE(m_collection.ContainsId(id));
   auto readLock = m_collection.WriteLock(id);
   ASSERT_EQ("Hello", readLock->Get_substuff1());
   ASSERT_EQ(123, readLock->Get_substuff2());
   ASSERT_EQ(false, readLock->Get_substuff3());
}


TEST_F(CollectionTests, range_for_loop_test)
{
   VerifyRangeLoopHas({}, "Empty array");

   AddObjectsWithIds({ 1, 5, 10 });
   VerifyRangeLoopHas({ 1, 5, 10 }, "added 3 objects 1,5,10");

   // Note the order is strictly ascending
   AddObjectsWithIds({ 2, 4, 6 });
   VerifyRangeLoopHas({ 1, 2, 4, 5, 6, 10 }, "added 3 objects 2,4,6");

   m_collection.Delete(10);
   VerifyRangeLoopHas({ 1, 2, 4, 5, 6 }, "Removed 10");

   m_collection.Delete(1);
   VerifyRangeLoopHas({ 2, 4, 5, 6 }, "Removed 1");

   m_collection.Delete(6);
   VerifyRangeLoopHas({ 2, 4, 5 }, "Removed 6");

   AddObjectWithId(32);
   VerifyRangeLoopHas({ 2, 4, 5, 32 }, "Added 32");

   AddObjectWithId(100);
   VerifyRangeLoopHas({ 2, 4, 5, 32, 100 }, "Added 100");

   m_collection.clear();
   VerifyRangeLoopHas({}, "Emptied array");
}

TEST_F(CollectionTests, range_for_loop_for_const)
{
   size_t count = 0;
   const auto& const_copy = m_collection;
   for (auto& sub : const_copy)
   {
      sub.Get_substuff1_or("Hi");
      count++;
   }
}

TEST_F(CollectionTests, range_for_loop_for_non_const)
{
   int32_t count = 0;

   m_collection.Post(1);
   m_collection.Post(2);
   m_collection.Post(3);

   for (auto& sub : m_collection)
   {
      sub.Set_substuff2(count);
      count++;
   }

   count = 0;
   for (auto& sub : m_collection)
   {
      ASSERT_EQ(count, sub.Get_substuff2_or(1000));
      count++;
   }
}

TEST_F(CollectionTests, validate_directly_on_collection)
{
   AddObjectsWithIds({ 1, 5, 10 });

   // Note that TestValidator only fails when we fill in our m_validationError string with non empty value
   auto subscriptionHandle = m_collection.OnChangeToObject([&](auto& info) { return TestCallback(info); });
   auto validationHandle = m_collection.ValidateWith([&](auto& message) { return TestValidator(message); });
   ASSERT_TRUE(validationHandle) << "Can't validate";

   // Can change things when validator works...
   auto result = m_collection.RestPatchString("/1", R"({"substuff1":"Hello"})");
   ASSERT_EQ(200, result.GetCode()) << result.GetDetails();
   ASSERT_EQ(1, m_validationCount) << "we were supposed to be validating this transaction";
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("1").c_str());
   ASSERT_EQ(1, m_notificationCount) << "if change was successful, we should have notified";

   // Can't change things when validator fails...
   m_validationError = "Housten, we have a problem";
   m_collection.RestPatchString("/1", R"({"substuff1":"World"})");
   ASSERT_EQ(2, m_validationCount) << "check we were validating this transaction";
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("/1").c_str());
   ASSERT_EQ(1, m_notificationCount) << "if change was NOT successful, we should NOT have notified";

   // and we still haven't affected our underlying resource
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("/1").c_str());

   // allow validation to pass again...
   m_validationError = "";

   // change value now works...
   m_collection.RestPatchString("/1", R"({"substuff1":"World"})");
   ASSERT_EQ(3, m_validationCount); // check we were validating this transaction
   ASSERT_STREQ(R"({"id":1,"substuff1":"World"})", m_collection.ToJSON("/1").c_str());
}

TEST_F(CollectionTests, validate_only_occurs_on_changes)
{
   AddObjectsWithIds({ 1, 5, 10 });

   // Note that TestValidator only fails when we fill in our m_validationError string with non empty value
   auto subscriptionHandle = m_collection.ValidateWith([&](auto& message)-> bool { return TestValidator(message); });
   ASSERT_TRUE(subscriptionHandle) << "Can't validate";

   // Can change things when validator works...
   m_collection.RestPatchString("1/substuff1", "\"Hello\"");
   ASSERT_EQ(1, m_validationCount); // check we were validating this transaction
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("/1").c_str());

   m_collection.RestPatchString("1/substuff1", "\"Hello\"");
   ASSERT_EQ(1, m_validationCount); // check we didn't validate when nothing changed

   m_collection.RestPatchString("1/substuff1", "\"Changed\"");
   ASSERT_EQ(2, m_validationCount); // check we did validate when something changed
}

TEST_F(CollectionTests, CollectionCapacity)
{
   m_collection.clear();
   
   for (size_t i = 0; i < m_collection.capacity(); i++)
   {
      ASSERT_TRUE(m_collection.Post().Commit()) << "Failed to add on attempt " << i << std::endl;
   }

   ASSERT_FALSE(m_collection.Post()) << "Successful post when collection is full!";
}

TEST_F(CollectionTests, persistence_subscriber_gets_called_on_update)
{
   std::string path;
   std::string action;
   jude_id_t   id = 0;

   Collection<TagsTest> collection("Test");

   collection.Post(12);

   collection.SubscribeToAllPaths(
      "/Test",
      [&](const std::string& pathThatChanged, const jude::Notification<jude::Object>& info)
      {
         action = "UPDATED";
         if (info.IsNew())
         {
            action = "ADDED";
         }
         else if (info.IsDeleted())
         {
            action = "DELETED";
         }
         id = info->Id();
         path = pathThatChanged;
      },
      FieldMask::ForPersistence_DeltasOnly,
      NotifyQueue::Immediate
   );

   collection[12]->Set_privateStatus(2);
   ASSERT_STREQ(action.c_str(), "");
   ASSERT_STREQ(path.c_str(), "");
   ASSERT_EQ(id, 0);
   
   collection[12]->Set_privateConfig(23);
   ASSERT_STREQ(action.c_str(), "UPDATED");
   ASSERT_STREQ(path.c_str(), "/Test/12");
   ASSERT_EQ(id, 12);

   // reset
   action = ""; path = ""; id = 0;

   collection[12]->Set_privateConfig(23);
   ASSERT_STREQ(action.c_str(), "");
   ASSERT_STREQ(path.c_str(), "");
   ASSERT_EQ(id, 0);

   collection[12]->Set_publicConfig(23);
   ASSERT_STREQ(action.c_str(), "UPDATED");
   ASSERT_STREQ(path.c_str(), "/Test/12");
   ASSERT_EQ(id, 12);
}

TEST_F(CollectionTests, persistence_subscriber_gets_called_on_add)
{
   std::string path;
   std::string action;
   jude_id_t   id = 0;

   Collection<TagsTest> collection("Test");

   collection.SubscribeToAllPaths(
      "/Test",
      [&](const std::string& pathThatChanged, const jude::Notification<jude::Object>& info)
      {
         action = "UPDATED";
         if (info.IsNew())
         {
            action = "ADDED";
         }
         else if (info.IsDeleted())
         {
            action = "DELETED";
         }
         id = info->Id();
         path = pathThatChanged;
      },
      FieldMask::ForPersistence_DeltasOnly,
      NotifyQueue::Immediate
   );

   collection.Post(12);
   ASSERT_STREQ(action.c_str(), "ADDED");
   ASSERT_STREQ(path.c_str(), "/Test/12");
   ASSERT_EQ(id, 12);
}

TEST_F(CollectionTests, persistence_subscriber_gets_called_on_delete)
{
   std::string path;
   std::string action;
   jude_id_t   id = 0;

   Collection<TagsTest> collection("Test");
   collection.Post(12);

   collection.SubscribeToAllPaths(
      "/Test",
      [&](const std::string& pathThatChanged, const jude::Notification<jude::Object>& info)
      {
         action = "UPDATED";
         if (info.IsNew())
         {
            action = "ADDED";
         }
         else if (info.IsDeleted())
         {
            action = "DELETED";
         }
         id = info->Id();
         path = pathThatChanged;
      },
      FieldMask::ForPersistence_DeltasOnly,
      NotifyQueue::Immediate
   );

   collection.Delete(12);
   ASSERT_STREQ(action.c_str(), "DELETED");
   ASSERT_STREQ(path.c_str(), "/Test/12");
   ASSERT_EQ(id, 12);
}

TEST_F(CollectionTests, collection_RestAPI_with_search)
{
   // Note that we are testing the Rest GET method with the ToJSON method as that is a simple wrapper for the GET
   AddObjectWithId(1, "Hello");
   AddObjectWithId(2, nullptr, 1234);
   AddObjectWithId(3, nullptr, 0, true);

   EXPECT_STREQ(R"({"1":{"id":1,"substuff1":"Hello"},"2":{"id":2,"substuff2":1234},"3":{"id":3,"substuff3":true}})", m_collection.ToJSON("/").c_str());

   EXPECT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("/1").c_str());
   EXPECT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_collection.ToJSON("/*substuff1=Hello").c_str());
   m_collection.RestPatchString("/*substuff1=Hello/substuff2", "789");
   EXPECT_STREQ(R"({"id":1,"substuff1":"Hello","substuff2":789})", m_collection.ToJSON("/*substuff1=Hello").c_str());

   EXPECT_STREQ(R"({"id":2,"substuff2":1234})", m_collection.ToJSON("/2").c_str());
   EXPECT_STREQ(R"({"id":2,"substuff2":1234})", m_collection.ToJSON("/*substuff2=1234").c_str());
   m_collection.RestDelete("/*substuff2=1234");
   EXPECT_STREQ("#ERROR: Not Found", m_collection.ToJSON("/*substuff2=1234").c_str());
   EXPECT_STREQ("#ERROR: Not Found", m_collection.ToJSON("/2").c_str());

   EXPECT_STREQ(R"({"id":3,"substuff3":true})", m_collection.ToJSON("/3").c_str());
   EXPECT_STREQ(R"({"id":3,"substuff3":true})", m_collection.ToJSON("/*substuff3=true").c_str());

   EXPECT_STREQ("#ERROR: Not Found", m_collection.ToJSON("/*substuff1=BlahBlah").c_str());
   EXPECT_STREQ("#ERROR: Not Found", m_collection.ToJSON("/*substuff2=NotFound").c_str());
   EXPECT_STREQ("#ERROR: Not Found", m_collection.ToJSON("/*substuff3=Invalid").c_str());
}
