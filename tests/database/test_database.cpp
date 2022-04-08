#include <gtest/gtest.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Database.h"
#include "jude/database/Resource.h"
#include "jude/database/Collection.h"

using namespace jude;

class DatabaseTests : public JudeTestBase
{
public:
   static constexpr size_t DefaultCapacity = 64;
   static constexpr jude_user_t DefaultAccess = jude_user_Public;

   TestDatabase m_database;
   size_t m_notificationCount;
   size_t m_notificationOfNewObjects;
   size_t m_notificationOfDeletedObjects;

   const char* NotFound     =         "Not Found";
   const char* JsonNotFound = "#ERROR: Not Found";

   const char* MethodNotAllowed     =         "Method Not Allowed";
   const char* JsonMethodNotAllowed = "#ERROR: Method Not Allowed";

   DatabaseTests()
      : m_notificationCount(0)
      , m_notificationOfNewObjects(0)
      , m_notificationOfDeletedObjects(0)
   {
   }

   void AssertDatabaseJSON(Database& db, const char *fullpath, std::string expectedJSON)
   {
      ASSERT_EQ(db.ToJSON(fullpath), expectedJSON);
   }

   void AssertNotFound(const char* fullpath)
   {
      ASSERT_EQ(m_database.ToJSON(fullpath), JsonNotFound) << "Expected fullpath '" << fullpath << "' to NOT exist in DB";
   }

   void AssertPathExists(const char* fullpath)
   {
      ASSERT_NE(m_database.ToJSON(fullpath), JsonNotFound) << "Expected fullpath '" << fullpath << "' to exist in DB";
   }

   void TestCallback(const Notification<Object>& notification)
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

   void CheckPublishOnAddAndRemove(Database &db, const char* path, bool subscribed_to_id)
   {
      auto expectedNotificationCount = m_notificationCount;
      auto expectedAddCount = m_notificationOfNewObjects;
      auto expectedDeletedCount = m_notificationOfDeletedObjects;

      auto result = db.RestPostString(path, "{}");
      ASSERT_TRUE(result.IsOK()) << "Error trying to POST new resource to " << path;
      auto id = result.GetCreatedObjectId();

      if (subscribed_to_id)
      {
         expectedNotificationCount++;
         expectedAddCount++;
      }
      ASSERT_EQ(expectedNotificationCount, m_notificationCount) << (subscribed_to_id ? "subscribed to id, should have notified on Add" : "not subscribed to 'id' did not expect notify on Add");
      ASSERT_EQ(expectedAddCount, m_notificationOfNewObjects) << (subscribed_to_id ? "subscribed to id, should have notified a *NEW* resource" : "not subscribed to 'id' did not expect notify on Add");
      ASSERT_EQ(expectedDeletedCount, m_notificationOfDeletedObjects) << (subscribed_to_id ? "subscribed to id, should have notified a *NEW* resource" : "not subscribed to 'id' did not expect notify on Add");

      char deletePath[256];
      snprintf(deletePath, sizeof(deletePath), "%s/%" PRIjudeID, path, id);
      ASSERT_REST_OK(db.RestDelete(deletePath));

      if (subscribed_to_id)
      {
         expectedNotificationCount++;
         expectedDeletedCount++;
      }
      ASSERT_EQ(expectedNotificationCount, m_notificationCount) << (subscribed_to_id ? "subscribed to id, should have notified on Remove" : "not subscribed to 'id' did not expect notify on Remove");
      ASSERT_EQ(expectedAddCount, m_notificationOfNewObjects) << (subscribed_to_id ? "subscribed to id, unexpected Add notify count after delete" : "not subscribed to 'id', unexpected Add notify count after delete");
      ASSERT_EQ(expectedDeletedCount, m_notificationOfDeletedObjects) << (subscribed_to_id ? "subscribed to id, should have notified a *REMOVED* resource" : "not subscribed to 'id', unexpected Delete notify count after delete");
   }

   void Check_No_Publish_For_Field(Database& db, const char* fieldName, const char* value1, const char* value2)
   {
      Check_Publish_For_Field(db, fieldName, value1, value2, false);
   }

   void Check_Publish_For_Field(Database &db, const char* fullpath, const char* value1, const char* value2, bool expectPublish = true)
   {
      auto expectedNotificationCount = m_notificationCount;

      ASSERT_REST_OK(db.RestPutString(fullpath, value1));

      if (expectPublish) expectedNotificationCount++;
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
         << "setting " << fullpath << " should " << (expectPublish ? "" : "not ") << "cause notification on publish";

      ASSERT_REST_OK(db.RestPatchString(fullpath, value1));
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
         << "setting " << fullpath << " to same value should not notify";

      ASSERT_REST_OK(db.RestPatchString(fullpath, value2));
      if (expectPublish) expectedNotificationCount++;
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
         << "setting " << fullpath << " to different value should " << (expectPublish ? "notify" : "not notify as no subscription");
   }

   void Check_Const_Functions_Do_Not_Publish(Database &db)
   {
      db.ToJSON();
      ASSERT_EQ(0, m_notificationCount) << "const functions should not created notifications on publish";
   }
};

TEST_F(DatabaseTests, simple_add_objects)
{
   AssertNotFound("repeats");
   AssertNotFound("optionals");

   Resource<AllRepeatedTypes> repeatsFirst("repeats");
   Resource<AllRepeatedTypes> repeatsCopy("repeats");
   Resource<AllOptionalTypes> optional("optionals");

   ASSERT_TRUE(m_database.AddToDB(repeatsFirst));
   AssertPathExists("repeats");
   AssertPathExists("/repeats");
   AssertPathExists("repeats/");
   AssertPathExists("/repeats/");
   AssertPathExists("///repeats////");

   AssertNotFound("optionals");
   AssertNotFound("/optionals");
   AssertNotFound("optionals/");
   AssertNotFound("/optionals/");
   AssertNotFound("///optionals///");

   ASSERT_FALSE(m_database.AddToDB(repeatsCopy)) << "Should not be able to add to same fullpath";

   ASSERT_TRUE(m_database.AddToDB(optional));
   AssertPathExists("repeats");
   AssertPathExists("optionals");

   ASSERT_FALSE(m_database.AddToDB(optional)) << "Should not be able to add same resource twice";
   AssertPathExists("optionals");
}

TEST_F(DatabaseTests, simple_add_collections)
{
   AssertNotFound("repeats");
   AssertNotFound("optionals");

   Collection<AllRepeatedTypes> repeat("repeats", DefaultCapacity, DefaultAccess);
   Collection<AllRepeatedTypes> repeatCopy("repeats", DefaultCapacity, DefaultAccess);
   Collection<AllOptionalTypes> optional("optionals", DefaultCapacity, DefaultAccess);

   ASSERT_TRUE(m_database.AddToDB(repeat));
   AssertPathExists("repeats");
   AssertPathExists("/repeats");
   AssertPathExists("repeats/");
   AssertPathExists("/repeats/");
   AssertPathExists("///repeats////");

   AssertNotFound("optionals");
   AssertNotFound("/optionals");
   AssertNotFound("optionals/");
   AssertNotFound("/optionals/");
   AssertNotFound("///optionals///");

   ASSERT_FALSE(m_database.AddToDB(repeatCopy)) << "Should not be able to add to same fullpath";

   ASSERT_TRUE(m_database.AddToDB(optional));
   AssertPathExists("repeats");
   AssertPathExists("optionals");

   ASSERT_FALSE(m_database.AddToDB(optional)) << "Should NOT be able to add same resource in two places (alias)";
   AssertPathExists("optionals");
}

TEST_F(DatabaseTests, collections_and_objects_share_the_same_namespace)
{
   Collection<AllRepeatedTypes> collection1("repeats", DefaultCapacity, DefaultAccess);
   Collection<AllRepeatedTypes> collection1Copy("repeats", DefaultCapacity, DefaultAccess);
   Collection<AllOptionalTypes> collection2("optionals", DefaultCapacity, DefaultAccess);
   Collection<AllOptionalTypes> collection2Copy("optionals", DefaultCapacity, DefaultAccess);
   Resource<AllRepeatedTypes> resource1("repeat");
   Resource<AllOptionalTypes> resource2("optional");

   ASSERT_TRUE (m_database.AddToDB(collection1));
   ASSERT_FALSE(m_database.AddToDB(collection1Copy));
   ASSERT_TRUE (m_database.AddToDB(collection2));
   ASSERT_FALSE(m_database.AddToDB(collection2Copy));

   ASSERT_TRUE(m_database.AddToDB(resource1));
   ASSERT_TRUE(m_database.AddToDB(resource2));
}

TEST_F(DatabaseTests, Rest_operations_on_root_not_allowed)
{
   PopulatedDB db;
   
   db.SetAllowGlobalRestGet(false);
   AssertDatabaseJSON(db, "/", JsonMethodNotAllowed);
   db.subDB.SetAllowGlobalRestGet(true);
   AssertDatabaseJSON(db.subDB, "/", "{\"collection\":{\"6\":{\"id\":6}},\"resource\":{\"id\":3,\"substuff1\":\"Hello\"}}");

   ASSERT_REST_FAILS_WITH(db.RestDelete(""), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestDelete("/"), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestPostString("", "{}"), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestPostString("/", "{}"), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestPutString("", "{}"), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestPutString("/", "{}"), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestPatchString("", "{}"), MethodNotAllowed);
   ASSERT_REST_FAILS_WITH(db.RestPatchString("/", "{}"), MethodNotAllowed);
}

TEST_F(DatabaseTests, Rest_GET_aka_ToJSON)
{
   PopulatedDB db;

   // Paths into resource 1
   AssertDatabaseJSON(db, "/resource1/", R"({"id":1,"substuff1":"Hello"})");
   AssertDatabaseJSON(db, "/resource1/substuff1", R"("Hello")");
   AssertDatabaseJSON(db, "/resource1/id", "1");
   AssertDatabaseJSON(db, "/resource1/substuff2", JsonNotFound);

   // Paths into resource 2
   AssertDatabaseJSON(db, "/resource2/", R"({"id":2,"substuff2":1234})");
   AssertDatabaseJSON(db, "/resource2/substuff1", JsonNotFound);
   AssertDatabaseJSON(db, "/resource2/id", "2");
   AssertDatabaseJSON(db, "/resource2/substuff2", "1234");
   
   // Paths into collection 1 
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4}})");
   AssertDatabaseJSON(db, "/collection1/substuff1", JsonNotFound);
   AssertDatabaseJSON(db, "/collection1/4", R"({"id":4})");
   AssertDatabaseJSON(db, "/collection1/4/id", "4");
   AssertDatabaseJSON(db, "/collection1/4/substuff1", JsonNotFound);

   // Paths into collection 2 
   AssertDatabaseJSON(db, "/collection2/", R"({"5":{"id":5}})");
   AssertDatabaseJSON(db, "/collection2/substuff1", JsonNotFound);
   AssertDatabaseJSON(db, "/collection2/5", R"({"id":5})");
   AssertDatabaseJSON(db, "/collection2/5/id", "5");
   AssertDatabaseJSON(db, "/collection2/5/substuff2", JsonNotFound);

   // paths into sub db
   AssertDatabaseJSON(db, "/subDB/resource", R"({"id":3,"substuff1":"Hello"})");
   AssertDatabaseJSON(db, "/subDB/collection", R"({"6":{"id":6}})");
}

TEST_F(DatabaseTests, Rest_POST)
{
   PopulatedDB db;

   // Create a resource via a Restful POST...
   uuid_counter = 7;
   ASSERT_EQ(uuid_counter, 7);// double check our next resource will have id == 7

   // Post blank resource...
   ASSERT_REST_OK(db.RestPostString("/collection1", R"({})"));
   AssertDatabaseJSON(db, "/collection1/7", R"({"id":7})"); // in collection 1
   AssertDatabaseJSON(db, "/collection2/7", JsonNotFound); // but not in collection 2

   // Post resource with some values...
   ASSERT_REST_OK(db.RestPostString("/collection1", R"({"substuff2":1234})"));
   AssertDatabaseJSON(db, "/collection1/8", R"({"id":8,"substuff2":1234})");

   // Post resource with id filled in - it should replace it with a new one...
   ASSERT_REST_OK(db.RestPostString("/collection1", R"({"id":999, "substuff1":"Hello"})"));
   AssertDatabaseJSON(db, "/collection1/9", R"({"id":9,"substuff1":"Hello"})");

   // Post to another collection - it should keep uuid unique..
   ASSERT_REST_OK(db.RestPostString("/collection2", R"({"substuff1":"World"})"));
   AssertDatabaseJSON(db, "/collection2/10", R"({"id":10,"substuff1":"World"})");
}

TEST_F(DatabaseTests, Rest_PATCH)
{
   PopulatedDB db;

   ASSERT_REST_FAIL(db.RestPatchString("/collection1", R"({"substuff2":1234})"));
   ASSERT_REST_FAIL(db.RestPatchString("/collection1/123", R"({"substuff2":1234})"));

   ASSERT_REST_OK(db.RestPatchString("/collection1/4", R"({"substuff2":1234})"));
   ASSERT_REST_OK(db.RestPatchString("/collection2/5", R"({"substuff1":"Hello"})"));

   AssertDatabaseJSON(db, "/collection1/4", R"({"id":4,"substuff2":1234})"); // just resource 5
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4,"substuff2":1234}})"); // all of collection 1

   ASSERT_REST_OK(db.RestPatchString("/collection1/4/substuff3", "true"));
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4,"substuff2":1234,"substuff3":true}})");
   ASSERT_REST_OK(db.RestPatchString("/collection1/4/", R"({"substuff1":"Hello"})"));
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4,"substuff1":"Hello","substuff2":1234,"substuff3":true}})");

   ASSERT_REST_FAILS_WITH(db.RestPatchString("/collection2/5/substuff3", "1234"), "substuff3: Expected true, false or null");
   ASSERT_REST_OK(db.RestPatchString("/collection2/5/substuff1", R"("World")"));
   AssertDatabaseJSON(db, "/collection2/", R"({"5":{"id":5,"substuff1":"World"}})");
}

TEST_F(DatabaseTests, Rest_PUT)
{
   PopulatedDB db;

   ASSERT_REST_FAIL(db.RestPutString("/collection1", R"({"substuff2":1234})"));
   ASSERT_REST_FAIL(db.RestPutString("/collection1/123", R"({"substuff2":1234})"));
   ASSERT_REST_OK(  db.RestPutString("/collection1/4", R"({"substuff2":1234})"));
   ASSERT_REST_OK(  db.RestPutString("/collection2/5", R"({"substuff1":"Hello"})"));

   AssertDatabaseJSON(db, "/collection1/4", R"({"id":4,"substuff2":1234})"); // just resource 5
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4,"substuff2":1234}})"); // all of collection 1

   ASSERT_REST_OK(db.RestPutString("/collection1/4/substuff3", "true"));
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4,"substuff2":1234,"substuff3":true}})");
   
   // Note this is different to PATCH because we replace the entire resource 3 with our new JSON...
   ASSERT_REST_OK(db.RestPutString("/collection1/4/", R"({"substuff1":"Hello"})"));
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4,"substuff1":"Hello"}})");

   ASSERT_REST_FAILS_WITH(db.RestPutString("/collection2/5/substuff3", "1234"), "substuff3: Expected true, false or null");
   ASSERT_REST_OK(db.RestPutString("/collection2/5/substuff1", R"("World")"));
   AssertDatabaseJSON(db, "/collection2/", R"({"5":{"id":5,"substuff1":"World"}})");
}

TEST_F(DatabaseTests, Rest_DELETE)
{
   PopulatedDB db;

   // Can't delete top level database objects (resources or collections)
   ASSERT_REST_FAIL(db.RestDelete("/collection1"));
   ASSERT_REST_FAIL(db.RestDelete("/collection2/"));
   ASSERT_REST_FAIL(db.RestDelete("resource1/"));
   ASSERT_REST_FAIL(db.RestDelete("resource2"));
   ASSERT_REST_FAIL(db.RestDelete("i_dont_exist"));

   // Add two more resources to each collection
   db.collection1.Post(7);
   db.collection1.Post(8);
   ASSERT_EQ(3, db.collection1.count());

   db.collection2.Post(9);
   db.collection2.Post(10);
   ASSERT_EQ(3, db.collection2.count());

   // Delete from collection
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4},"7":{"id":7},"8":{"id":8}})");
   ASSERT_REST_FAIL(db.RestDelete("/collection1/2"));
   ASSERT_REST_OK(db.RestDelete("/collection1/7"));
   AssertDatabaseJSON(db, "/collection1/", R"({"4":{"id":4},"8":{"id":8}})");

   // Delete inside a resource from a collection...
   ASSERT_REST_OK(db.RestPatchString("collection2/9/substuff2", "1234"));
   AssertDatabaseJSON(db, "/collection2/", R"({"5":{"id":5},"9":{"id":9,"substuff2":1234},"10":{"id":10}})");
   ASSERT_REST_OK(db.RestDelete("/collection2/9/substuff2"));
   AssertDatabaseJSON(db, "/collection2/", R"({"5":{"id":5},"9":{"id":9},"10":{"id":10}})");

   // Delete inside top level resource...
   ASSERT_REST_OK(db.RestPatchString("resource1/substuff3", "false"));
   AssertDatabaseJSON(db, "/resource1/", R"({"id":1,"substuff1":"Hello","substuff3":false})");
   ASSERT_REST_OK(db.RestDelete("resource1/substuff3"));
   AssertDatabaseJSON(db, "/resource1/", R"({"id":1,"substuff1":"Hello"})");

   // Delete inside subDB collection...
   // std::cout << db.ToJSON() << std::endl;

   ASSERT_REST_OK(db.RestPatchString("subDB/resource/substuff3", "false"));
   AssertDatabaseJSON(db, "subDB/resource", R"({"id":3,"substuff1":"Hello","substuff3":false})");
   ASSERT_REST_OK(db.RestDelete("subDB/resource/substuff3"));
   AssertDatabaseJSON(db, "subDB/resource", R"({"id":3,"substuff1":"Hello"})");
}

TEST_F(DatabaseTests, cannot_subscribe_to_root)
{
   PopulatedDB db;

   ASSERT_FALSE(db.OnChangeToPath("",  [](const auto&) {}));
   ASSERT_FALSE(db.OnChangeToPath("/", [](const auto&) {}));
   ASSERT_FALSE(db.OnChangeToPath("//", [](const auto&) {}));
}

TEST_F(DatabaseTests, subscribe_to_all)
{
   PopulatedDB db;

   auto subscriptionHandle1 = db.OnChangeToPath("/resource1", [&](auto& i) { TestCallback(i); });
   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";
   auto subscriptionHandle2 = db.OnChangeToPath("/collection1", [&](auto& i) { TestCallback(i); });
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";

   ASSERT_EQ(0, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish(db);
   Check_Publish_For_Field(db, "/resource1/substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field(db, "/resource1/substuff2", "123", "456");
   Check_Publish_For_Field(db, "/resource1/substuff3", "false", "true");

   Check_Publish_For_Field(db, "/collection1/4/substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field(db, "/collection1/4/substuff2", "123", "456");
   Check_Publish_For_Field(db, "/collection1/4/substuff3", "false", "true");

   CheckPublishOnAddAndRemove(db, "/collection1/", true);
}

TEST_F(DatabaseTests, subscribe_to_multiple_fields)
{
   PopulatedDB db;

   auto subscriptionHandle1 = db.OnChangeToPath("/resource1",
      [&](auto& i) { TestCallback(i); },
      {
         SubMessage::Index::substuff1,
         SubMessage::Index::substuff3
      });
   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";

   auto subscriptionHandle2 = db.OnChangeToPath("/collection1",
      [&](auto& i) { TestCallback(i); },
      {
         SubMessage::Index::substuff1,
         SubMessage::Index::substuff3
      });
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";

   ASSERT_EQ(0, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish(db);
   Check_Publish_For_Field   (db, "/resource1/substuff1", R"("Hello")", R"("World")");
   Check_No_Publish_For_Field(db, "/resource1/substuff2", "123", "456");
   Check_Publish_For_Field   (db, "/resource1/substuff3", "false", "true");

   Check_Publish_For_Field   (db, "/collection1/4/substuff1", R"("Hello")", R"("World")");
   Check_No_Publish_For_Field(db, "/collection1/4/substuff2", "123", "456");
   Check_Publish_For_Field   (db, "/collection1/4/substuff3", "false", "true");
   CheckPublishOnAddAndRemove(db, "/collection1/", false);
}

TEST_F(DatabaseTests, subscribe_to_single)
{
   PopulatedDB db;

   auto subscriptionHandle1 = db.OnChangeToPath("/resource1",
      [&](auto& i) { TestCallback(i); }, 
      {
         SubMessage::Index::substuff2
      });
   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";

   auto subscriptionHandle2 = db.OnChangeToPath("/collection1",
      [&](auto& i) { TestCallback(i); },
      {
         SubMessage::Index::substuff2
      });
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";

   ASSERT_EQ(0, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish(db);
   Check_No_Publish_For_Field(db, "/resource1/substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field   (db, "/resource1/substuff2", "123", "456");
   Check_No_Publish_For_Field(db, "/resource1/substuff3", "false", "true");

   Check_No_Publish_For_Field(db, "/collection1/4/substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field   (db, "/collection1/4/substuff2", "123", "456");
   Check_No_Publish_For_Field(db, "/collection1/4/substuff3", "false", "true");
   uuid_counter = 100;
   CheckPublishOnAddAndRemove(db, "/collection1/", false);

   subscriptionHandle1.Unsubscribe();
   Check_No_Publish_For_Field(db, "/resource1/substuff2", "123", "456");

   subscriptionHandle2.Unsubscribe();
   Check_No_Publish_For_Field(db, "/collection1/4/substuff2", "123", "456");
}

TEST_F(DatabaseTests, subscribe_to_single_path)
{
   PopulatedDB db;

   auto subscriptionHandle1 = db.OnChangeToPath("/resource1/substuff2", [&](auto& i) { TestCallback(i); });
   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";

   auto subscriptionHandle2 = db.OnChangeToPath("/collection1/+/substuff2", [&](auto& i) { TestCallback(i); });
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";

   ASSERT_EQ(0, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish(db);
   Check_No_Publish_For_Field(db, "/resource1/substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field   (db, "/resource1/substuff2", "123", "456");
   Check_No_Publish_For_Field(db, "/resource1/substuff3", "false", "true");

   Check_No_Publish_For_Field(db, "/collection1/4/substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field   (db, "/collection1/4/substuff2", "123", "456");
   Check_No_Publish_For_Field(db, "/collection1/4/substuff3", "false", "true");
   uuid_counter = 100;
   CheckPublishOnAddAndRemove(db, "/collection1/", true);

   subscriptionHandle1.Unsubscribe();
   Check_No_Publish_For_Field(db, "/resource1/substuff2", "123", "456");

   subscriptionHandle2.Unsubscribe();
   Check_No_Publish_For_Field(db, "/collection1/4/substuff2", "123", "456");
}

TEST_F(DatabaseTests, subscribe_to_add_or_remove)
{
   PopulatedDB db;

   auto subscriptionHandle1 = db.OnObjectAdded([&](auto& i) { TestCallback(i); }, "/collection1");
   auto subscriptionHandle2 = db.OnObjectDeleted([&](auto& i) { TestCallback(i); }, "/collection1");
   CheckPublishOnAddAndRemove(db, "/collection1/", true);
   CheckPublishOnAddAndRemove(db, "/collection2/", false);

   auto subscriptionHandle3 = db.OnObjectAdded([&](auto& i) { TestCallback(i); }, "/collection2");
   auto subscriptionHandle4 = db.OnObjectDeleted([&](auto& i) { TestCallback(i); }, "/collection2");
   CheckPublishOnAddAndRemove(db, "/collection1/", true);
   CheckPublishOnAddAndRemove(db, "/collection2/", true);

   subscriptionHandle1.Unsubscribe();
   subscriptionHandle2.Unsubscribe();
   CheckPublishOnAddAndRemove(db, "/collection1/", false);
   CheckPublishOnAddAndRemove(db, "/collection2/", true);

   subscriptionHandle3.Unsubscribe();
   subscriptionHandle4.Unsubscribe();
   CheckPublishOnAddAndRemove(db, "/collection1/", false);
   CheckPublishOnAddAndRemove(db, "/collection2/", false);
}

TEST_F(DatabaseTests, persistence_subscriber_gets_called)
{
   std::string path;
   std::string action;
   jude_id_t   id = 0;

   PopulatedDB db;

   db.SubscribeToAllPaths(
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

   db.collection1.Post(12);
   ASSERT_STREQ(action.c_str(), "ADDED");
   ASSERT_STREQ(path.c_str(), "/Test/collection1/12");
   ASSERT_EQ(id, 12);

   db.subDB.resource->Set_substuff1("Changed");
   ASSERT_STREQ(action.c_str(), "UPDATED");
   ASSERT_STREQ(path.c_str(), "/Test/subDB/resource");

   db.subDB.collection.Post(7)->Set_substuff1("Changed");
   ASSERT_STREQ(action.c_str(), "ADDED");
   ASSERT_STREQ(path.c_str(), "/Test/subDB/collection/7");

}

TEST_F(DatabaseTests, database_locks)
{
   std::shared_ptr<jude::Mutex> sharedMutex = std::make_shared<jude::Mutex>();

   TestDB db("LockTest", jude_user_Public, sharedMutex);

   ASSERT_EQ(0, sharedMutex->GetLockDepth());

   {
      auto lock = db.resource1.WriteLock();
      ASSERT_EQ(1, sharedMutex->GetLockDepth());
   }

   ASSERT_EQ(0, sharedMutex->GetLockDepth());

   {
      auto lock = db.collection1.Post(1);
      ASSERT_EQ(1, sharedMutex->GetLockDepth());
   }

   ASSERT_EQ(0, sharedMutex->GetLockDepth());

   // transient locks do not get kept open
   auto value = db.collection1[1]->Get_substuff2();
   auto value2 = db.resource1->Get_substuff1();
   ASSERT_EQ(0, sharedMutex->GetLockDepth());

}
