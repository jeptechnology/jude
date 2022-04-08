#include <gtest/gtest.h>

#include "jude/restapi/jude_browser.h"
#include "../core/test_base.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "jude/database/Resource.h"

using namespace jude;

class NotifyImmediatelyOnChangeTests : public JudeTestBase
{
public:
   size_t notificationCount;

   Resource<AllOptionalTypes> resource;
   AllOptionalTypes transaction;

   NotifyImmediatelyOnChangeTests()
      : resource("UUT")
      , transaction(nullptr)
   {
      Options::NotifyImmediatelyOnChange = true;

      transaction = resource.WriteLock();

      notificationCount = 0;
   }

   ~NotifyImmediatelyOnChangeTests()
   {
      Options::NotifyImmediatelyOnChange = false;
   }

};

#define CHECK_PUBLISH_ON_CHANGES(type, value1, value2)                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Should not notify on subscription: " << #type;     \
   resource.WriteLock(); \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on unchanged object should not notify: " << #type; \
   transaction.Set_ ## type(value1); \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on changed object should call : " << #type;\
   transaction.Set_ ## type(value1);                                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #type "() with same value should not publish"; \
   transaction.Set_ ## type(value2);                                                                      \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #type "() with new value should publish";  \
   transaction.Clear_ ## type();                                                                          \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Clear_" #type "() should publish";         \

#define CHECK_NO_PUBLISH_ON_CHANGES(type, value1, value2)                                                   \
   transaction.Set_ ## type(value1);                                                                      \
   transaction.Set_ ## type(value2);                                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #type "() should not publish as not subscribed field"; \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Multiple publishes should not notofy again";       \
   transaction.Clear_ ## type();                                                                          \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Clear_" #type "() should not publish as not subscribed field"; \

TEST_F(NotifyImmediatelyOnChangeTests, notification_sends_correct_object)
{
   auto callback = [&](const Notification<Object>& notification) 
   {
      notificationCount++;

      ASSERT_TRUE(notification);
      ASSERT_FALSE(notification.IsNew());
      ASSERT_FALSE(notification.IsDeleted());
      ASSERT_FALSE(notification.IsNew());
      ASSERT_TRUE(notification->IsIdAssigned());
      ASSERT_TRUE(notification->Has(jude::AllOptionalTypes::Index::string_type));
      ASSERT_FALSE(notification->Has(jude::AllOptionalTypes::Index::bool_type));
      ASSERT_FALSE(notification->Has(jude::AllOptionalTypes::Index::uint16_type));
      ASSERT_EQ(notification->GetField("string_type"), "\"Hi\"");
   };

   auto subscriptionHandle = resource.OnChangeToObject(callback);

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   transaction.Set_string_type("Hi");
   
   ASSERT_EQ(1, notificationCount);
}

TEST_F(NotifyImmediatelyOnChangeTests, notification_sends_correct_typed_object)
{
   auto subscriptionHandle = resource.OnChange([&](const jude::Notification<jude::AllOptionalTypes>& notification)
      {
         notificationCount++;

         ASSERT_TRUE(notification);
         ASSERT_FALSE(notification.IsNew());
         ASSERT_FALSE(notification.IsDeleted());
         ASSERT_EQ(notification.Source(), resource.WriteLock());
         ASSERT_FALSE(notification.IsNew());
         ASSERT_TRUE(notification->IsIdAssigned());
         ASSERT_TRUE(notification->Has_string_type());
         ASSERT_FALSE(notification->Has_bool_type());
         ASSERT_FALSE(notification->Has_uint64_type());
         ASSERT_EQ(notification->GetField("string_type"), "\"Hi\"");
      }
   );

   transaction.Set_string_type("Hi");

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";
   ASSERT_EQ(1, notificationCount);
}

TEST_F(NotifyImmediatelyOnChangeTests, can_subscribe_to_all_fields)
{
   auto subscriptionHandle = resource.OnChangeToObject(
      [&](const jude::Notification<jude::Object>&) {
         notificationCount++;
      }
   );
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   CHECK_PUBLISH_ON_CHANGES(bool_type,  true, false);
   CHECK_PUBLISH_ON_CHANGES(int8_type,  -5, 5);
   CHECK_PUBLISH_ON_CHANGES(int16_type, -5, 5);
   CHECK_PUBLISH_ON_CHANGES(int32_type, -5, 5);
   CHECK_PUBLISH_ON_CHANGES(int64_type, -5, 5);
   CHECK_PUBLISH_ON_CHANGES(uint8_type, 15, 5);
   CHECK_PUBLISH_ON_CHANGES(uint16_type, 15, 5);
   CHECK_PUBLISH_ON_CHANGES(uint32_type, 15, 5);
   CHECK_PUBLISH_ON_CHANGES(uint64_type, 15, 5);
   CHECK_PUBLISH_ON_CHANGES(enum_type, jude::TestEnum::First, jude::TestEnum::Second);
   CHECK_PUBLISH_ON_CHANGES(string_type, "Hello", "World");
}

TEST_F(NotifyImmediatelyOnChangeTests, can_subscribe_to_one_field)
{
   auto subscriptionHandle = resource.OnChangeToObject(
      [&](const jude::Notification<jude::Object>&) {
         notificationCount++;
      },
      { jude::AllOptionalTypes::Index::enum_type }
   );
   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   CHECK_NO_PUBLISH_ON_CHANGES(bool_type, true, false);
   CHECK_NO_PUBLISH_ON_CHANGES(int8_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(int16_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(int32_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(int64_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint8_type, 15, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint16_type, 15, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint32_type, 15, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint64_type, 15, 5);
   CHECK_PUBLISH_ON_CHANGES(enum_type, jude::TestEnum::First, jude::TestEnum::Second);
   CHECK_NO_PUBLISH_ON_CHANGES(string_type, "Hello", "World");
}

TEST_F(NotifyImmediatelyOnChangeTests, can_subscribe_to_multiple_fields)
{
   auto subscriptionHandle = resource.OnChangeToObject(
      [&](const jude::Notification<jude::Object>&) {
         notificationCount++;
      },
      {
         jude::AllOptionalTypes::Index::int16_type,
         jude::AllOptionalTypes::Index::uint64_type,
         jude::AllOptionalTypes::Index::string_type,
         jude::AllOptionalTypes::Index::bool_type
      }
   );

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   unsigned expectedNotifyCount = 0;

   CHECK_PUBLISH_ON_CHANGES(bool_type, true, false);
   CHECK_NO_PUBLISH_ON_CHANGES(int8_type, -5, 5);
   CHECK_PUBLISH_ON_CHANGES(int16_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(int32_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(int64_type, -5, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint8_type, 15, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint16_type, 15, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(uint32_type, 15, 5);
   CHECK_PUBLISH_ON_CHANGES(uint64_type, 15, 5);
   CHECK_NO_PUBLISH_ON_CHANGES(enum_type, jude::TestEnum::First, jude::TestEnum::Second);
   CHECK_PUBLISH_ON_CHANGES(string_type, "Hello", "World");
}
