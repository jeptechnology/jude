#include <gtest/gtest.h>

#include "jude/restapi/jude_browser.h"
#include "../core/test_base.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "jude/database/Resource.h"

using namespace jude;

class ObjectSubscriptionTests : public JudeTestBase
{
public:
   size_t notificationCount;

   Resource<AllOptionalTypes> optionalTypes;

   ObjectSubscriptionTests()
      : optionalTypes("UUT")
   {
      notificationCount = 0;
   }
};

#define CHECK_PUBLISH_ON_CHANGES(type, value1, value2)                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Should not notify on subscription: " << #type;     \
   optionalTypes.WriteLock(); \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on unchanged object should not notify: " << #type; \
   optionalTypes->Set_ ## type(value1); \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Publish on changed object should call : " << #type;\
   optionalTypes->Set_ ## type(value1);                                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #type "() with same value should not publish"; \
   optionalTypes->Set_ ## type(value2);                                                                      \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #type "() with new value should publish";  \
   optionalTypes->Clear_ ## type();                                                                          \
   expectedNotifyCount++;                                                                                   \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Clear_" #type "() should publish";         \

#define CHECK_NO_PUBLISH_ON_CHANGES(type, value1, value2)                                                   \
   optionalTypes->Set_ ## type(value1);                                                                      \
   optionalTypes->Set_ ## type(value2);                                                                      \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Set_" #type "() should not publish as not subscribed field"; \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Multiple publishes should not notofy again";       \
   optionalTypes->Clear_ ## type();                                                                          \
   ASSERT_EQ(expectedNotifyCount, notificationCount) << "Calling Clear_" #type "() should not publish as not subscribed field"; \

TEST_F(ObjectSubscriptionTests, notification_sends_correct_object)
{
   auto callback = [&](const Notification<Object>& notification) 
   {
      notificationCount++;

      ASSERT_TRUE(notification);
      ASSERT_TRUE(notification.IsNew());
      ASSERT_FALSE(notification.IsDeleted());
      ASSERT_TRUE(notification.IsNew());
      ASSERT_EQ(notification->Id(), 123);
      ASSERT_TRUE(notification->IsIdAssigned());
      ASSERT_TRUE(notification->Has(jude::AllOptionalTypes::Index::string_type));
      ASSERT_FALSE(notification->Has(jude::AllOptionalTypes::Index::bool_type));
      ASSERT_FALSE(notification->Has(jude::AllOptionalTypes::Index::uint16_type));
   };

   auto subscriptionHandle = optionalTypes.OnChangeToObject(callback);

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";

   optionalTypes->Set_string_type("Hi").AssignId(123);
   
   ASSERT_EQ(1, notificationCount);
}

TEST_F(ObjectSubscriptionTests, notification_sends_correct_typed_object)
{
   auto subscriptionHandle = optionalTypes.OnChange([&](const jude::Notification<jude::AllOptionalTypes>& notification)
      {
         notificationCount++;

         ASSERT_TRUE(notification);
         ASSERT_TRUE(notification.IsNew());
         ASSERT_FALSE(notification.IsDeleted());
         ASSERT_EQ(notification.Source(), optionalTypes.WriteLock());
         ASSERT_TRUE(notification.IsNew());
         ASSERT_EQ(notification->Id(), 123);
         ASSERT_TRUE(notification->IsIdAssigned());
         ASSERT_TRUE(notification->Has_string_type());
         ASSERT_FALSE(notification->Has_bool_type());
         ASSERT_FALSE(notification->Has_uint64_type());
      }
   );

   optionalTypes->Set_string_type("Hi").AssignId(123);

   ASSERT_TRUE(subscriptionHandle) << "Can't subscribe";
   ASSERT_EQ(1, notificationCount);
}

TEST_F(ObjectSubscriptionTests, can_subscribe_to_all_fields)
{
   auto subscriptionHandle = optionalTypes.OnChangeToObject(
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

TEST_F(ObjectSubscriptionTests, can_subscribe_to_one_field)
{
   auto subscriptionHandle = optionalTypes.OnChangeToObject(
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

TEST_F(ObjectSubscriptionTests, can_subscribe_to_multiple_fields)
{
   auto subscriptionHandle = optionalTypes.OnChangeToObject(
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
