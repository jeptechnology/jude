#include <gtest/gtest.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Resource.h"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std::chrono_literals;
using namespace jude;

class ResourceTests : public JudeTestBase
{
public:
   Resource<SubMessage> m_resource;
   size_t m_notificationCount;
   size_t m_notificationOfNewObjects;
   size_t m_notificationOfDeletedObjects;

   std::string m_validationError;
   size_t m_validationCount;

   ResourceTests()
       : m_resource("MyIndividual"), m_notificationCount(0), m_notificationOfNewObjects(0), m_notificationOfDeletedObjects(0), m_validationError(""), m_validationCount(0)
   {
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

   void TestCallback(const Notification<SubMessage> &notification)
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

   void GenericTestCallback(const Notification<Object> &notification)
   {
      m_notificationCount++;
      if (notification)
      {
         // if its not a deleted object then it is a root object as it lives inside our individual as a top level resource
         ASSERT_FALSE(notification->Parent().IsOK());
      }
   }

   void Check_No_Publish_For_Field(const char *fieldName, const char *value1, const char *value2)
   {
      Check_Publish_For_Field(fieldName, value1, value2, false);
   }

   void Check_Publish_For_Field(const char *fieldName, const char *value1, const char *value2, bool expectPublish = true)
   {
      auto expectedNotificationCount = m_notificationCount;

      ASSERT_REST_OK(m_resource.RestPatchString(fieldName, value1));

      if (expectPublish)
         expectedNotificationCount++;
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
          << "setting " << fieldName << " should " << (expectPublish ? "" : "not ") << "cause notification on publish";

      ASSERT_REST_OK(m_resource.RestPatchString(fieldName, value1));
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
          << "setting " << fieldName << " to same value should not notify";

      ASSERT_REST_OK(m_resource.RestPatchString(fieldName, value2));
      if (expectPublish)
         expectedNotificationCount++;
      ASSERT_EQ(expectedNotificationCount, m_notificationCount)
          << "setting " << fieldName << " to different value should " << (expectPublish ? "notify" : "not notify as no subscription");
   }

   void Check_Const_Functions_Do_Not_Publish()
   {
      auto expectedCount = m_notificationCount;
      m_resource.GetAccessLevel(jude::CRUD::READ);
      m_resource.GetName();
      m_resource.ToJSON();
      ASSERT_EQ(expectedCount, m_notificationCount) << "const functions should not create new notifications";
   }
};

TEST_F(ResourceTests, resource_is_initialised_with_id_on_init)
{
   ASSERT_STREQ(R"({"id":1})", m_resource.ToJSON().c_str());
}

TEST_F(ResourceTests, cleared_field_returns_defaulted_value)
{
   m_resource->Set_substuff1("Hello");
   ASSERT_STREQ("Hello", m_resource->Get_substuff1().c_str());
   m_resource->Clear_substuff1();
   ASSERT_STREQ("", m_resource->Get_substuff1().c_str());

   m_resource->Set_substuff2(1234);
   ASSERT_EQ(1234, m_resource->Get_substuff2());
   m_resource->Clear_substuff2();
   ASSERT_EQ(0, m_resource->Get_substuff2());

   m_resource->Set_substuff3(true);
   ASSERT_EQ(true, m_resource->Get_substuff3());
   m_resource->Clear_substuff3();
   ASSERT_EQ(false, m_resource->Get_substuff3());

}

TEST_F(ResourceTests, resource_copies_data_on_access)
{
   auto readCopy = m_resource->Clone();
   ASSERT_STREQ("", readCopy.Get_substuff1().c_str());

   // Note that "World" does not appear in the individual yet...
   ASSERT_STREQ(R"({"id":1})", m_resource.ToJSON().c_str());

   // When I change the resource value to "Hello" and patch it back to the individual...
   m_resource->Set_substuff1("Hello");

   // Then I don't see the "Hello" in the old read transaction (which made a copy)
   ASSERT_STREQ("", readCopy.Get_substuff1().c_str());

   // Then I see the "Hello" in the individual...
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());
}

TEST_F(ResourceTests, resource_Rest_GET_aka_ToJSON)
{
   // Note that we are testing the Rest GET method with the ToJSON method as that is a simple wrapper for the GET
   m_resource->Set_substuff1("Hello").Set_substuff2(1234);

   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello","substuff2":1234})", m_resource.ToJSON().c_str());

   // Paths into resource
   ASSERT_STREQ(R"("Hello")", m_resource.ToJSON("/substuff1").c_str());
   ASSERT_STREQ(R"(1234)", m_resource.ToJSON("/substuff2").c_str());
   ASSERT_STREQ(R"(#ERROR: Not Found)", m_resource.ToJSON("/substuff3").c_str());
}

TEST_F(ResourceTests, resource_Rest_POST_fails_on_root_path)
{
   // Restful POST not allowed on root...
   ASSERT_REST_FAIL(m_resource.RestPostString("/", R"({"id":1,"substuff2":1234})"));
}

TEST_F(ResourceTests, resource_Rest_PATCH)
{
   // Edit resource directly...
   m_resource->Set_substuff1("Hello");
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());

   ASSERT_FALSE(m_resource.RestPatchString("/2", R"({"substuff2":1234})").IsOK()) << "Should not be able to patch non existent path in individual";

   ASSERT_TRUE(m_resource.RestPatchString("/", R"({"substuff2":1234})").IsOK());

   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello","substuff2":1234})", m_resource.ToJSON().c_str());

   ASSERT_TRUE(m_resource.RestPatchString("", R"({"substuff2":5678,"substuff1":"World"})").IsOK());
   ASSERT_STREQ(R"({"id":1,"substuff1":"World","substuff2":5678})", m_resource.ToJSON().c_str());
}

TEST_F(ResourceTests, resource_Rest_PUT)
{
   // Edit resource directly...
   m_resource->Set_substuff1("Hello");
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());

   ASSERT_FALSE(m_resource.RestPutString("/2", R"({"substuff2":1234})").IsOK()) << "Should not be able to put non existent resource in individual";

   ASSERT_TRUE(m_resource.RestPutString("/", R"({"substuff2":1234})").IsOK());

   // Note: "Hello" should be removed due to PUT replacing everything given (apart from id!
   ASSERT_STREQ(R"({"id":1,"substuff2":1234})", m_resource.ToJSON().c_str());

   ASSERT_TRUE(m_resource.RestPutString("", R"({"substuff2":5678,"substuff1":"World"})").IsOK());
   ASSERT_STREQ(R"({"id":1,"substuff1":"World","substuff2":5678})", m_resource.ToJSON().c_str());
}

TEST_F(ResourceTests, resource_Rest_DELETE)
{
   m_resource->Set_substuff1("Hello");

   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());

   ASSERT_FALSE(m_resource.RestDelete("/").IsOK()) << "Should not be able to delete the entire individual";
   ASSERT_FALSE(m_resource.RestDelete("/substuff2").IsOK()) << "Should not be able to delete non existent field";

   ASSERT_TRUE(m_resource.RestDelete("substuff1").IsOK());
   ASSERT_STREQ(R"({"id":1})", m_resource.ToJSON().c_str());
}

TEST_F(ResourceTests, subscribe_to_all)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_resource.OnChange([&](auto& n) { TestCallback(n); });
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_Publish_For_Field("substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field("substuff2", "123", "456");
   Check_Publish_For_Field("substuff3", "false", "true");
}

TEST_F(ResourceTests, subscribe_to_multiple_fields)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_resource.OnChange(
      [&](auto& n) { TestCallback(n); },
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
}

TEST_F(ResourceTests, subscribe_to_single)
{
   size_t expectedNotificationCount = m_notificationCount;
   auto subscriptionHandle = m_resource.OnChange(
      [&](auto& n) { TestCallback(n); },
      { SubMessage::Index::substuff2 });
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "Subscribing should not created notifications on publish";

   Check_Const_Functions_Do_Not_Publish();
   Check_No_Publish_For_Field("substuff1", R"("Hello")", R"("World")");
   Check_Publish_For_Field("substuff2", "123", "456");
   Check_No_Publish_For_Field("substuff3", "false", "true");
}

TEST_F(ResourceTests, validate_directly_on_individual)
{
   // Note that TestValidator only fails when we fill in our m_validationError string with non empty value
   auto subscriptionHandle = m_resource.OnChange([&](auto& n) { TestCallback(n); });
   auto validationHandle = m_resource.ValidateWith([&](auto& r) { return TestValidator(r); });
   ASSERT_TRUE(validationHandle) << "Can't validate";

   // Can change things when validator works...
   m_resource.TransactionLock()->Set_substuff1("Hello");
   ASSERT_EQ(1, m_validationCount) << "we were supposed to be validating this transaction";
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());
   ASSERT_EQ(1, m_notificationCount) << "if change was successful, we should have notified";

   // Can't change things when validator fails...
   m_validationError = "Housten, we have a problem";
   m_resource.TransactionLock()->Set_substuff1("World");
   ASSERT_EQ(2, m_validationCount) << "check we were validating this transaction";
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());
   ASSERT_EQ(1, m_notificationCount) << "if change was NOT successful, we should NOT have notified";

   // Check we can manually validate as we go...
   {
      auto lock = m_resource.TransactionLock();
      lock->Set_substuff1("World!");
      auto result = lock.Commit();
      ASSERT_FALSE(result);
      ASSERT_EQ(3, m_validationCount); // check we were validating this transaction
      ASSERT_STREQ("Housten, we have a problem", result.GetDetails().c_str());
   }
   // Note that we stop validating and commiting transactions once we have done so manually
   ASSERT_EQ(3, m_validationCount);
   // and we still haven't affected our underlying resource
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());

   // allow validation to pass again...
   m_validationError = "";
   // change value now works...
   m_resource.TransactionLock()->Set_substuff1("World");
   ASSERT_EQ(4, m_validationCount); // check we were validating this transaction
   ASSERT_STREQ(R"({"id":1,"substuff1":"World"})", m_resource.ToJSON().c_str());

   auto expectedNotificationCount = m_notificationCount;

   // Check we can abort a transaction
   {
      auto lock = m_resource.TransactionLock();
      lock->Set_substuff1("Abort!!!");
      lock.Abort();
   }
   ASSERT_EQ(4, m_validationCount) << "due to abort, we were NOT validating this transaction";
   ASSERT_EQ(expectedNotificationCount, m_notificationCount) << "due to abort, we were NOT commiting this transaction";
   ASSERT_STREQ(R"({"id":1,"substuff1":"World"})", m_resource.ToJSON().c_str());
}

TEST_F(ResourceTests, validate_only_occurs_on_changes)
{
   // Note that TestValidator only fails when we fill in our m_validationError string with non empty value
   auto subscriptionHandle = m_resource.ValidateWith([&](auto& r) { return TestValidator(r); });
   ASSERT_TRUE(subscriptionHandle) << "Can't validate";

   // Can change things when validator works...
   m_resource.TransactionLock()->Set_substuff1("Hello");
   ASSERT_EQ(1, m_validationCount); // check we were validating this transaction
   ASSERT_STREQ(R"({"id":1,"substuff1":"Hello"})", m_resource.ToJSON().c_str());

   m_resource.TransactionLock()->Set_substuff1("Hello");
   ASSERT_EQ(1, m_validationCount); // check we didn't validate when nothing changed

   m_resource.TransactionLock()->Set_substuff1("Changed");
   ASSERT_EQ(2, m_validationCount); // check we did validate when something changed
}

TEST_F(ResourceTests, write_lock_prevents_other_write_locks)
{
   std::mutex m;
   std::condition_variable cv, unlock_signal;

   unsigned startCount = 0;
   unsigned lockCount = 0;
   unsigned unlockCount = 0;

   auto transactionFunction = [&] {
      {
         std::unique_lock<std::mutex> lk(m);
         startCount++;
         // std::cout << "startCount: " << startCount;
         cv.notify_all();
      }

      {
         auto lock = m_resource.WriteLock();

         std::unique_lock<std::mutex> lk(m);
         lockCount++;
         // std::cout << "lockCount: " << startCount;
         cv.notify_all();
         unlock_signal.wait_for(lk, 5s);
      }

      {
         std::unique_lock<std::mutex> lk(m);
         unlockCount++;
         // std::cout << "unlockCount: " << startCount;
         cv.notify_all();
      }
   };

   auto t1 = std::make_unique<std::thread>(transactionFunction);
   auto t2 = std::make_unique<std::thread>(transactionFunction);

   {
      std::unique_lock<std::mutex> lk(m);

      // wait for two threads to start
      cv.wait_for(lk, 5s, [&] { return startCount >= 2; });

      // wait for at least one thread to lock
      cv.wait_for(lk, 5s, [&] { return lockCount >= 1; });
   }

   EXPECT_EQ(1, lockCount);

   // See if other thread manages to lock
   {
      std::unique_lock<std::mutex> lk(m);
      cv.wait_for(lk, 100ms);
   }

   EXPECT_EQ(1, lockCount);

   {
      std::unique_lock<std::mutex> lk(m);
      
      // Tell thread to unlock
      unlock_signal.notify_all();
      
      // wait for unlock
      cv.wait_for(lk, 5s, [&] { return unlockCount >= 1; });

      // wait for second thread to lock
      cv.wait_for(lk, 5s, [&] { return lockCount >= 2; });
      
      // Tell second thread to unlock
      unlock_signal.notify_all();
   }

   t1->join();
   t2->join();

   // both threads have started, locked and unlocked
   EXPECT_EQ(2, startCount);
   EXPECT_EQ(2, lockCount);
   EXPECT_EQ(2, unlockCount);
}

TEST_F(ResourceTests, chained_subscribers_do_no_corrupt_memory)
{
   auto subscriptionHandle1 = m_resource.OnChange(
      [&](auto&) {
         m_resource->Set_substuff2(123);
      },
      { jude::SubMessage::Index::substuff1 }
   );

   auto subscriptionHandle2 = m_resource.OnChange(
      [&](auto&) {
         m_resource->Set_substuff3(false);
      },
      { jude::SubMessage::Index::substuff2 }
   );

   auto subscriptionHandle3 = m_resource.OnChange(
      [&](auto&) {
         ASSERT_EQ("Hello", m_resource->Get_substuff1());
         ASSERT_EQ(123, m_resource->Get_substuff2());
         ASSERT_EQ(false, m_resource->Get_substuff3());
      },
      { jude::SubMessage::Index::substuff3 }
   );

   m_resource->Set_substuff1("Hello");

   auto readLock = m_resource.WriteLock();
   ASSERT_EQ("Hello", readLock.Get_substuff1());
   ASSERT_EQ(123, readLock.Get_substuff2());
   ASSERT_EQ(false, readLock.Get_substuff3());
}

TEST_F(ResourceTests, persistence_subscriber_gets_called)
{
   std::string path;
   std::string action;

   Resource<TagsTest> individual("Test");

   auto subscriptionHandle1 = individual.SubscribeToAllPaths(
      "/Test/",
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

         path = pathThatChanged;
      },
      FieldMask::ForPersistence_DeltasOnly,
      NotifyQueue::Immediate
   );

   individual->Set_privateStatus(2);
   ASSERT_STREQ(action.c_str(), "");
   ASSERT_STREQ(path.c_str(), "");
   
   individual->Set_privateConfig(23);
   ASSERT_STREQ(action.c_str(), "UPDATED");
   ASSERT_STREQ(path.c_str(), "/Test/");

   action = ""; path = "";

   individual->Set_privateConfig(23);
   ASSERT_STREQ(action.c_str(), "");
   ASSERT_STREQ(path.c_str(), "");

   individual->Set_publicConfig(23);
   ASSERT_STREQ(action.c_str(), "UPDATED");
   ASSERT_STREQ(path.c_str(), "/Test/");

}
