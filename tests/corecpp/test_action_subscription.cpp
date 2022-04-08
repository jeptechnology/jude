#include <gtest/gtest.h>

#include "jude/restapi/jude_browser.h"
#include "../core/test_base.h"
#include "autogen/alltypes_test/ActionTest.h"
#include "jude/database/Resource.h"

using namespace jude;

class ActionSubscriberTests : public JudeTestBase
{
public:
   size_t notificationCount;
   
   Resource<ActionTest> myObject;

   ActionSubscriberTests()
      : myObject("Test")
   {
      notificationCount = 0;
   }
};

#define CHECK_PUBLISH_ON_CHANGES(field, value1, value2)                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Should not notify on subscription: " << #field;     \
   myObject.WriteLock(); \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on unchanged object should not notify: " << #field; \
   myObject->Set_ ## field(value1); \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on changed object should call : " << #field;\
   myObject.WriteLock(); \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Multiple publishes should not notify again: " << #field;  \
   myObject->Set_ ## field(value1);                                                                      \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #field "() with same value should publish again for action fields"; \
   myObject->Set_ ## field(value2);                                                                      \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #field "() with new value should publish";  \
   myObject->Clear_ ## field();                                                                          \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Clear_" #field "() should not publish on an action field";         \

#define CHECK_NO_PUBLISH_ON_CHANGES(field, value1, value2)                                                   \
   myObject->Set_ ## field(value1);                                                                      \
   myObject->Set_ ## field(value2);                                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #field "() should not publish as not subscribed field"; \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Multiple publishes should not notofy again";       \
   myObject->Clear_ ## field();                                                                          \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Clear_" #field "() should not publish as not subscribed field"; \

TEST_F(ActionSubscriberTests, notification_sends_correct_object)
{
   auto callback = [&](const jude::Notification<jude::Object>& notification) 
   {
      notificationCount++;

      ASSERT_TRUE(notification);
      ASSERT_TRUE(notification.IsNew());
      ASSERT_FALSE(notification.IsDeleted());
      ASSERT_TRUE(notification.IsNew());
      ASSERT_TRUE(notification->IsIdAssigned());
      ASSERT_EQ(notification->Id(), 123);
      ASSERT_TRUE(notification->Has(jude::ActionTest::Index::actionOnString));
      ASSERT_FALSE(notification->Has(jude::ActionTest::Index::actionOnObject));
      ASSERT_FALSE(notification->Has(jude::ActionTest::Index::actionOnBool));
      ASSERT_FALSE(notification->Has(jude::ActionTest::Index::actionOnInteger));
   };

   auto subscriptionHandle = myObject.OnChangeToObject(callback);

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   myObject->Set_actionOnString("Hi").AssignId(123);
   
   ASSERT_EQ(1, notificationCount);
}

TEST_F(ActionSubscriberTests, notification_sends_correct_typed_object)
{
   auto subscriptionHandle = myObject.OnChange([&](const jude::Notification<jude::ActionTest>& notification)
      {
         notificationCount++;

         ASSERT_TRUE(notification);
         ASSERT_TRUE(notification.IsNew());
         ASSERT_FALSE(notification.IsDeleted());
         ASSERT_EQ(notification.Source(), myObject.WriteLock());
         ASSERT_TRUE(notification.IsNew());
         ASSERT_EQ(notification->Id(), 123);
         ASSERT_TRUE(notification->IsIdAssigned());
         ASSERT_TRUE(notification->Has_actionOnString());
         ASSERT_FALSE(notification->Has_actionOnBool());
         ASSERT_FALSE(notification->Has_actionOnInteger());
         ASSERT_FALSE(notification->Has_actionOnObject());
      }
   );

   myObject->Set_actionOnString("Hi").AssignId(123);

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";
   ASSERT_EQ(1, notificationCount);
}

TEST_F(ActionSubscriberTests, can_subscribe_to_all_actions)
{
   auto subscriptionHandle = myObject.OnChangeToObject(
      [&](const jude::Notification<jude::Object>& ) {
         notificationCount++;
      }
   );
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   myObject.WriteLock();

   CHECK_PUBLISH_ON_CHANGES(actionOnBool,  true, false);
   CHECK_PUBLISH_ON_CHANGES(actionOnInteger,  -5, 5);
   CHECK_PUBLISH_ON_CHANGES(actionOnString, "Hello", "World");
}

TEST_F(ActionSubscriberTests, can_subscribe_to_one_action)
{
   auto subscriptionHandle = myObject.OnChangeToObject(
      [&](const jude::Notification<jude::Object>& ) {
         notificationCount++;
      },
      { jude::ActionTest::Index::actionOnBool }
   );
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   CHECK_PUBLISH_ON_CHANGES(actionOnBool, true, false);
   CHECK_NO_PUBLISH_ON_CHANGES(actionOnInteger, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(actionOnString, "Hello", "World");
}

TEST_F(ActionSubscriberTests, can_subscribe_to_multiple_actions)
{
   auto subscriptionHandle = myObject.OnChangeToObject(
      [&](const jude::Notification<jude::Object>& ) {
         notificationCount++;
      },
      {
         jude::ActionTest::Index::actionOnBool,
         jude::ActionTest::Index::actionOnInteger
      }
   );

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   CHECK_PUBLISH_ON_CHANGES(actionOnBool, true, false);
   CHECK_NO_PUBLISH_ON_CHANGES(actionOnString, "Hello", "World");
   CHECK_PUBLISH_ON_CHANGES(actionOnInteger, -5, 5);
}

TEST_F(ActionSubscriberTests, can_subscribe_to_object_action)
{
   auto subscriptionHandle = myObject.OnChangeToObject(
      [&](const jude::Notification<jude::Object>& ) {
         notificationCount++;
      },
      jude::ActionTest::Index::actionOnObject
   );

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   
   {
      auto lock = myObject.WriteLock();
      lock.Get_actionOnObject().Set_bool_type(true);
      ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set() should not publish immediately";
   } // transaction finished here.. so we publish

   expectedNotifyCount++;                                                                                   
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on changed object should call";

   myObject.WriteLock();
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Multiple publishes should not notify again";  

   auto action = myObject->Get_actionOnObject().Clone();
   myObject->Set_actionOnObject(action);
   expectedNotifyCount++;
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set() with same value should publish again for action fields"; 

   action.Set_bool_type(false);
   myObject->Set_actionOnObject(action);
   expectedNotifyCount++;
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set() with new value should publish";  

   myObject->Clear_actionOnObject();
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling clear() should not publish on an action field";

   CHECK_NO_PUBLISH_ON_CHANGES(actionOnString, "Hello", "World");
}

TEST_F(ActionSubscriberTests, can_subscribe_to_action_on_queue)
{
   NotifyQueue queue("Test");

   auto subscriptionHandle = myObject.OnChange(
      [&](const jude::Notification<jude::ActionTest>& notification) {
         notificationCount++;
         ASSERT_TRUE(notification->Has_actionOnObject());
      },
      {
         jude::ActionTest::Index::actionOnObject
      },
      queue
   );

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   {
      auto lock = myObject.WriteLock();
      lock.Get_actionOnObject().Set_bool_type(true);
      ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set() should not publish immediately";
   }
   
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on queue should not notify immediately";

   queue.Process(0);
   expectedNotifyCount++;
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Notify should be called";

   myObject.WriteLock();
   queue.Process(0);
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Multiple publishes should not notify again";

   auto action = myObject->Get_actionOnObject().Clone();
   myObject->Set_actionOnObject(action);
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on queue should not notify immediately";

   queue.Process(0);
   expectedNotifyCount++;
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Notify should be called";

   action.Set_bool_type(false);
   myObject->Set_actionOnObject(action);
   queue.Process(0);
   expectedNotifyCount++;
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set() with new value should publish";

   myObject->Clear_actionOnObject();
   queue.Process(0);
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling clear() should not publish on an action field";

   CHECK_NO_PUBLISH_ON_CHANGES(actionOnString, "Hello", "World");
}
