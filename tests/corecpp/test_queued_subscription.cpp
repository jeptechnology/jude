#include <gtest/gtest.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Database.h"
#include "jude/database/Resource.h"
#include "jude/database/Collection.h"

using namespace jude;

class QueueTestDB : public Database
{
public:
   Resource<AllOptionalTypes> optional;
   Resource<AllRepeatedTypes> repeated;
   Collection<AllOptionalTypes> optionalCollection;
   Collection<AllRepeatedTypes> repeatedCollection;

   NotifyQueue queue;
   NotifyQueue queue2;

   QueueTestDB()
      : jude::Database("", jude_user_Public, std::make_shared<jude::Mutex>())
      , optional("opt")
      , repeated("rep")
      , optionalCollection("opts", 1024, jude_user_Public)
      , repeatedCollection("reps", 1024, jude_user_Public)
      , queue("Queue1ForTestDB")
      , queue2("Queue2ForTestDB")
   {
      InstallDatabaseEntry(optional);
      InstallDatabaseEntry(repeated);
      InstallDatabaseEntry(optionalCollection);
      InstallDatabaseEntry(repeatedCollection);
   }
};


class QueuedSubscriptionTests : public JudeTestBase
{
public:
   AllOptionalTypes m_optionalObject; // standalone resource
   AllRepeatedTypes m_repeatedObject; // standalone resource
   QueueTestDB      m_db;  // db full of collections and resources

   size_t m_notificationCount;
   size_t m_notificationOfNewObjects;
   size_t m_notificationOfDeletedObjects;

   std::string m_validationError;
   size_t m_validationCount;

   QueuedSubscriptionTests()
      : m_optionalObject(AllOptionalTypes::New())
      , m_repeatedObject(AllRepeatedTypes::New())
      , m_notificationCount(0)
      , m_notificationOfNewObjects(0)
      , m_notificationOfDeletedObjects(0)
      , m_validationError("")
      , m_validationCount(0)
   {
   }

   template<class T_Object>
   ValidationResult TestValidator(Notification<T_Object>&)
   {
      m_validationCount++;

      if (m_validationError.length() > 0)
      {
         return m_validationError;
      }
      return true;
   }

   template<class T_Object>
   void TestCallback(const Notification<T_Object>& notification)
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
         // if its not a deleted object then it is a root object as it lives inside our individual as a top level resource
         ASSERT_FALSE(notification->Parent().IsOK());
      }
   }

   void ProcessExpectedNotifications(bool expectedNotification, const char *path)
   {
      if (expectedNotification)
      {
         ASSERT_TRUE(m_db.queue.Process(0)) << "Expected a queued notification with " << path;
      }

      ASSERT_FALSE(m_db.queue.Process(0)) << "More queued notifications than expected: " << path;
   }

   void CheckQueuedPublishing(const char* path, const char* Value1, const char* Value2, size_t subscribersActive, size_t validatorsActive)
   {
      auto expectedNotifyCount = m_notificationCount;
      auto expectedValidationCount = m_validationCount;

      // PATCH the new Value...
      auto restResult = m_db.RestPatchString(path, Value1);
      ASSERT_TRUE(restResult.IsOK()) << "failure to patch " << path << " with " << Value1 << restResult.GetDetails();
      expectedValidationCount += validatorsActive;
      ASSERT_EQ(expectedValidationCount, m_validationCount) << "Validation should have taken place : " << path << " with " << Value1;
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should not have taken place yet : " << path << " with " << Value1;
      ProcessExpectedNotifications(subscribersActive > 0, path);

      expectedNotifyCount += subscribersActive;
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should now have been processed : " << path << " with " << Value1;

      // PATCH the *SAME* Value...
      restResult = m_db.RestPatchString(path, Value1);
      ASSERT_TRUE(restResult.IsOK()) << "failure to patch " << path << " with " << Value1 << restResult.GetDetails();
      ASSERT_EQ(expectedValidationCount, m_validationCount) << "No new validation should have taken place : " << path << " with " << Value1;
      ProcessExpectedNotifications(false, path);
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should not have taken place  - no change! : " << path << " with " << Value1;

      if (validatorsActive > 0)
      {
         // If we have validators, use them to prevent a PATCH ...
         m_validationError = "Block this change!";
         restResult = m_db.RestPatchString(path, Value2);
         ASSERT_FALSE(restResult.IsOK()) << "expecting patch to fail";
         expectedValidationCount += validatorsActive;
         ASSERT_EQ(expectedValidationCount, m_validationCount) << "Validation should have taken place : " << path << " with " << Value1;
         ProcessExpectedNotifications(false, path);
         m_validationError = "";
      }

      // PATCH the *other* Value...
      restResult = m_db.RestPatchString(path, Value2);
      ASSERT_TRUE(restResult.IsOK()) << "failure to patch " << path << " with " << Value2 << restResult.GetDetails();
      expectedValidationCount += validatorsActive;
      ASSERT_EQ(expectedValidationCount, m_validationCount) << "Validation should have taken place : " << path << " with " << Value1;
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should not have taken place yet : " << path << " with " << Value1;
      ProcessExpectedNotifications(subscribersActive > 0, path);
      expectedNotifyCount += subscribersActive;
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should now have been processed : " << path << " with " << Value1;

      // Clear the field...
      m_db.RestDelete(path);
      expectedValidationCount += validatorsActive;
      ASSERT_EQ(expectedValidationCount, m_validationCount) << "Validation should have taken place : " << path << " with " << Value1;
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should not have taken place yet : " << path << " with " << Value1;
      ProcessExpectedNotifications(subscribersActive > 0, path);
      expectedNotifyCount += subscribersActive;
      ASSERT_EQ(expectedNotifyCount, m_notificationCount) << "Queued notification should now have been processed : " << path << " with " << Value1;

      ProcessExpectedNotifications(false, path);
   }
};

TEST_F(QueuedSubscriptionTests, subscribe_to_individual_through_db_paths)
{
   auto subscriptionHandle1 = m_db.OnChangeToPath("/opt",
      [&](auto& info) { TestCallback<Object>(info); },
      {
         AllOptionalTypes::Index::int16_type,
         AllOptionalTypes::Index::uint64_type,
         AllOptionalTypes::Index::string_type,
         AllOptionalTypes::Index::bool_type
      },
      m_db.queue
   );

   auto subscriptionHandle2 = m_db.optional.OnChange(
      [&](auto& info) { TestCallback<AllOptionalTypes>(info); },
      {
         AllOptionalTypes::Index::uint16_type,
         AllOptionalTypes::Index::int64_type,
         AllOptionalTypes::Index::string_type // note string is in both subscribers
      },
      m_db.queue
   );

   auto validationHandle = m_db.optional.ValidateWith([&](Notification<AllOptionalTypes>& resource)
      { 
         return TestValidator<AllOptionalTypes>(resource); 
      });

   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";
   ASSERT_TRUE(validationHandle) << "Can't validate";

   // ------------------+ PATH to field      | Value 1 | Value 2 | # of subscribers | # of validators |
   CheckQueuedPublishing("/opt/int8_type",    "123",     "234",         0,             1);
   CheckQueuedPublishing("/opt/int16_type",   "123",     "234",         1,             1);
   CheckQueuedPublishing("/opt/int32_type",   "123",     "234",         0,             1);
   CheckQueuedPublishing("/opt/int64_type",   "123",     "234",         1,             1);
   CheckQueuedPublishing("/opt/uint8_type",   "123",     "234",         0,             1);
   CheckQueuedPublishing("/opt/uint16_type",  "123",     "234",         1,             1);
   CheckQueuedPublishing("/opt/uint32_type",  "123",     "234",         0,             1);
   CheckQueuedPublishing("/opt/uint64_type",  "123",     "234",         1,             1);
   CheckQueuedPublishing("/opt/bool_type",    "true",    "false",       1,             1);
   CheckQueuedPublishing("/opt/string_type",  "\"1\"",   "\"2\"",       2,             1);
   CheckQueuedPublishing("/opt/bytes_type",   "\"MQ==\"", "\"SGk=\"",   0,             1);
   CheckQueuedPublishing("/opt/submsg_type",  "{\"substuff2\":1}", "{\"substuff2\":2}", 0,         1);
   CheckQueuedPublishing("/opt/enum_type",    "1",       "2",           0,             1);
   CheckQueuedPublishing("/opt/bitmask_type", "1",       "2",           0,             1);
}

TEST_F(QueuedSubscriptionTests, subscribe_to_individual_through_lock)
{

   auto subscriptionHandle1 = m_db.optional.OnChangeToObject(
      [&](auto& info) { TestCallback<Object>(info); },
      {
         AllOptionalTypes::Index::int16_type,
         AllOptionalTypes::Index::uint64_type,
         AllOptionalTypes::Index::string_type,
         AllOptionalTypes::Index::bool_type
      },
      m_db.queue
   );

   auto subscriptionHandle2 = m_db.optional.OnChange(
      [&](auto& info) { TestCallback<AllOptionalTypes>(info); },
      {
         AllOptionalTypes::Index::uint16_type,
         AllOptionalTypes::Index::int64_type,
         AllOptionalTypes::Index::string_type // note string is in both subscribers
      },
      m_db.queue
   );

   auto validationHandle = m_db.optional.ValidateWith([&](Notification<AllOptionalTypes>& resource)
      {
         return TestValidator<AllOptionalTypes>(resource);
      }
   );

   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";
   ASSERT_TRUE(validationHandle) << "Can't validate";

   // ------------------+ PATH to field      | Value 1 | Value 2 | # of subscribers | # of validators |
   CheckQueuedPublishing("/opt/int8_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/int16_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opt/int32_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/int64_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opt/uint8_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/uint16_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opt/uint32_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/uint64_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opt/bool_type", "true", "false", 1, 1);
   CheckQueuedPublishing("/opt/string_type", "\"1\"", "\"2\"", 2, 1);
   CheckQueuedPublishing("/opt/bytes_type", "\"MQ==\"", "\"SGk=\"", 0, 1);
   CheckQueuedPublishing("/opt/submsg_type", "{\"substuff2\":1}", "{\"substuff2\":2}", 0, 1);
   CheckQueuedPublishing("/opt/enum_type", "1", "2", 0, 1);
   CheckQueuedPublishing("/opt/bitmask_type", "1", "2", 0, 1);

   subscriptionHandle2.Unsubscribe(); // NOTE: subscription handles are executable functions that will unsubscribe

   CheckQueuedPublishing("/opt/int8_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/int16_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opt/int32_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/int64_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/uint8_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/uint16_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/uint32_type", "123", "234", 0, 1);
   CheckQueuedPublishing("/opt/uint64_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opt/bool_type", "true", "false", 1, 1);
   CheckQueuedPublishing("/opt/string_type", "\"1\"", "\"2\"", 1, 1);
   CheckQueuedPublishing("/opt/bytes_type", "\"MQ==\"", "\"SGk=\"", 0, 1);
   CheckQueuedPublishing("/opt/submsg_type", "{\"substuff2\":1}", "{\"substuff2\":2}", 0, 1);
   CheckQueuedPublishing("/opt/enum_type", "1", "2", 0, 1);
   CheckQueuedPublishing("/opt/bitmask_type", "1", "2", 0, 1);
}

TEST_F(QueuedSubscriptionTests, subscribe_to_collection_through_db_paths)
{
   auto subscriptionHandle1 = m_db.OnChangeToPath("/opts",
      [&](auto& info) { TestCallback<Object>(info); },
      {
         AllOptionalTypes::Index::int16_type,
         AllOptionalTypes::Index::uint64_type,
         AllOptionalTypes::Index::string_type,
         AllOptionalTypes::Index::bool_type
      },
      m_db.queue
   );

   auto subscriptionHandle2 = m_db.optionalCollection.OnChange(
      [&](auto& info) { TestCallback<AllOptionalTypes>(info); },
      {
         AllOptionalTypes::Index::uint16_type,
         AllOptionalTypes::Index::int64_type,
         AllOptionalTypes::Index::string_type // note string is in both subscribers
      },
      m_db.queue
   );

   auto validationHandle = m_db.optionalCollection.ValidateWith([&](Notification<AllOptionalTypes>& resource)
      {
         return TestValidator<AllOptionalTypes>(resource);
      });

   ASSERT_TRUE(subscriptionHandle1) << "Can't subscribe";
   ASSERT_TRUE(subscriptionHandle2) << "Can't subscribe";
   ASSERT_TRUE(validationHandle) << "Can't validate";
 
   // Create a couple of resources in this collection and check their id's before testing further...
   auto id1 = m_db.RestPostString("opts/", "{}").GetCreatedObjectId();
   ASSERT_EQ(id1, 3);
   auto id2 = m_db.RestPostString("opts/", "{}").GetCreatedObjectId();
   ASSERT_EQ(id2, 4);

   // ------------------+ PATH to field      | Value 1 | Value 2 | # of subscribers | # of validators |
   CheckQueuedPublishing("/opts/3/int8_type",     "123",    "234", 0, 1);
   CheckQueuedPublishing("/opts/3/int16_type",    "123",    "234", 1, 1);
   CheckQueuedPublishing("/opts/3/int32_type",    "123",    "234", 0, 1);
   CheckQueuedPublishing("/opts/3/int64_type",    "123",    "234", 1, 1);
   CheckQueuedPublishing("/opts/3/uint8_type",    "123",    "234", 0, 1);
   CheckQueuedPublishing("/opts/3/uint16_type",   "123",    "234", 1, 1);
   CheckQueuedPublishing("/opts/3/uint32_type",   "123",    "234", 0, 1);
   // now do id 4
   CheckQueuedPublishing("/opts/4/uint64_type", "123", "234", 1, 1);
   CheckQueuedPublishing("/opts/4/bool_type", "true", "false", 1, 1);
   CheckQueuedPublishing("/opts/4/string_type", "\"1\"", "\"2\"", 2, 1);
   CheckQueuedPublishing("/opts/4/bytes_type", "\"MQ==\"", "\"SGk=\"", 0, 1);
   CheckQueuedPublishing("/opts/4/submsg_type", "{\"substuff2\":1}", "{\"substuff2\":2}", 0, 1);
   CheckQueuedPublishing("/opts/4/enum_type", "1", "2", 0, 1);
   CheckQueuedPublishing("/opts/4/bitmask_type", "1", "2", 0, 1);
}

TEST_F(QueuedSubscriptionTests, cant_subscribe_to_single_object_inside_collection)
{
   // Add 3 resources to collection
   m_db.optionalCollection.Post();
   m_db.optionalCollection.Post();
   m_db.optionalCollection.Post();

   // Attempt to subscribe to resource with id == 4
   auto subscriptionHandle = m_db.OnChangeToPath("/opts/4",
      [&](auto& info) { TestCallback<Object>(info); },
      {
         AllOptionalTypes::Index::int16_type,
         AllOptionalTypes::Index::int64_type,
         AllOptionalTypes::Index::string_type,
         AllOptionalTypes::Index::bool_type
      },
      m_db.queue
   );

   ASSERT_FALSE(subscriptionHandle) << "Shouldn't be able to subscribe to single resource";
}

TEST_F(QueuedSubscriptionTests, can_pause_and_play_a_queue)
{
   auto expectedCound = m_notificationCount;
   // Attempt to subscribe to resource with id == 4
   auto subscriptionHandle = m_db.optional.OnChangeToObject(
      [&](auto& info) { TestCallback<Object>(info); },
      {
         AllOptionalTypes::Index::int16_type,
         AllOptionalTypes::Index::int64_type,
         AllOptionalTypes::Index::string_type,
         AllOptionalTypes::Index::bool_type
      },
      m_db.queue
   );

   m_db.queue.Pause();

   m_db.optional->Set_bool_type(true);  // Should notify
   m_db.optional->Set_int32_type(1234); // Should not notify
   m_db.optional->Set_int16_type(1234); // Should notify
   ASSERT_EQ(expectedCound, m_notificationCount) << "Shouldn't expect notification on paused queue";

   m_db.queue.Play();
   ASSERT_EQ(expectedCound + 2, m_notificationCount) << "Expect all paused notifications to play out";

   m_db.optional->Set_int16_type(432); // Should notify
   m_db.queue.Process(0);
   ASSERT_EQ(expectedCound + 3, m_notificationCount) << "Should expect notification immediately on unpaused queue";
}
