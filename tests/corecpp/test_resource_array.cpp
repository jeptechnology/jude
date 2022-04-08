#include <gtest/gtest.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Resource.h"

using namespace jude;

class ObjectArrayTests : public JudeTestBase
{
public:
   size_t notificationCount;

   jude::AllRepeatedTypes arrayTypes;

   ObjectArrayTests()
      : arrayTypes(AllRepeatedTypes::New())
   {
      notificationCount = 0;
   }

   void TestCallback(const jude::Notification<jude::SubMessage>& notification)
   {
      notificationCount++;
      if (notification)
      {
         // if its not a deleted object then its parent should be our "arrayTypes"
         ASSERT_TRUE(notification->Parent());
      }
   };
};

TEST_F(ObjectArrayTests, object_array_is_empty_on_init)
{
   auto subMessages = arrayTypes.Get_submsg_types();

   ASSERT_EQ(0, subMessages.count());
   ASSERT_FALSE(subMessages.Find(1));
   ASSERT_FALSE(subMessages.RemoveAt(1));
}

TEST_F(ObjectArrayTests, object_array_add_increases_count)
{
   auto subMessages = arrayTypes.Get_submsg_types();
   auto result = subMessages.Add();
   ASSERT_TRUE(result.has_value());
   ASSERT_EQ(1, subMessages.count());
   ASSERT_STREQ(R"({"submsg_type":[{"id":1}]})", arrayTypes.ToJSON().c_str());
   ASSERT_TRUE(subMessages.Add().has_value());
   ASSERT_EQ(2, subMessages.count());
   ASSERT_STREQ(R"({"submsg_type":[{"id":1},{"id":2}]})", arrayTypes.ToJSON().c_str());
}

TEST_F(ObjectArrayTests, object_array_accesses_same_data)
{
   auto subMessages = arrayTypes.Get_submsg_types();
   auto result = subMessages.Add(123);
   ASSERT_TRUE(result.has_value());
   ASSERT_EQ(1, subMessages.count());
   ASSERT_FALSE(arrayTypes.Get_submsg_types().Find(1));
   ASSERT_TRUE(arrayTypes.Get_submsg_types().Find(123));
   arrayTypes.Get_submsg_types().Find(123)->Set_substuff1("Hello");
   ASSERT_STREQ(result->Get_substuff1().c_str(), "Hello");
   ASSERT_STREQ(R"({"submsg_type":[{"id":123,"substuff1":"Hello"}]})", arrayTypes.ToJSON().c_str());

}

TEST_F(ObjectArrayTests, object_array_add_limits_when_full)
{
   auto subMessages = arrayTypes.Get_submsg_types();

   EXPECT_EQ(32, subMessages.capacity());

   for (size_t i = 0; i < subMessages.capacity(); i++)
   {
      ASSERT_TRUE(subMessages.Add()) << "Could not add element " << i;
   }

   EXPECT_EQ(32, subMessages.count());
   ASSERT_TRUE(subMessages.Full()) << "array should be full";
   ASSERT_FALSE(subMessages.Add()) << "Should not be able to add another item";
   EXPECT_EQ(32, subMessages.count());
   ASSERT_TRUE(subMessages.Full());
}

TEST_F(ObjectArrayTests, object_array_remove)
{
   auto subMessages = arrayTypes.Get_submsg_types();

   auto first = subMessages.Add();
   auto second = subMessages.Add();
   auto last = subMessages.Add();

   ASSERT_STREQ(R"({"submsg_type":[{"id":1},{"id":2},{"id":3}]})", arrayTypes.ToJSON().c_str());
   subMessages.RemoveId(5);
   ASSERT_STREQ(R"({"submsg_type":[{"id":1},{"id":2},{"id":3}]})", arrayTypes.ToJSON().c_str());
   subMessages.RemoveId(first->Id());
   ASSERT_STREQ(R"({"submsg_type":[{"id":2},{"id":3}]})", arrayTypes.ToJSON().c_str());
   subMessages.RemoveId(second->Id());

   for (size_t i = 1; i < subMessages.capacity(); i++)
   {
      ASSERT_TRUE(subMessages.Add()) << "Could not add element " << i;
   }

   ASSERT_TRUE(subMessages.Full()) << "array should be full";
   ASSERT_FALSE(subMessages.Add()) << "Should not be able to add another item";
   subMessages.RemoveId(last->Id());
   EXPECT_EQ(subMessages.capacity() - 1, subMessages.count());
   ASSERT_TRUE(subMessages.Add()) << "Should be able to add another item after removal";
   ASSERT_TRUE(subMessages.Full());
}

#define CHECK_PUBLISH_ON_ADD_AND_REMOVE(subscribed_to_id) do { \
   auto sub = arrayTypes.Get_submsg_types().Add();                                                    \
   ASSERT_EQ(expectedNotificationCount, notificationCount) << "adding should not auto-publish";       \
   jude::Object::__PublishChanges(arrayTypes);   \
   if (subscribed_to_id) expectedNotificationCount++;                                                 \
   ASSERT_EQ(expectedNotificationCount, notificationCount) << (subscribed_to_id ? "subscribed to id, should have notified on Add" : "not subscribed to 'id' did not expect notify on Add"); \
   arrayTypes.Get_submsg_types().RemoveId(sub->Id());                                               \
   jude::Object::__PublishChanges(arrayTypes);   \
   if (subscribed_to_id) expectedNotificationCount++;                                                 \
   ASSERT_EQ(expectedNotificationCount, notificationCount) << (subscribed_to_id ? "subscribed to id, should have notified on Remove" : "not subscribed to 'id' did not expect notify on Remove"); \
} while (0)

#define CHECK_PUBLISH_ON_FIELD(field, value1, value2) do {                                            \
   auto sub = arrayTypes.Get_submsg_types().Add();                                                    \
   expectedNotificationCount = notificationCount;                                                     \
                                                                                                      \
   sub->Set_ ## field(value1);                                                                        \
   jude::Object::__PublishChanges(arrayTypes);   \
   expectedNotificationCount++;                                                                       \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
         << "setting " #field " should cause notification on publish";                                \
                                                                                                      \
   sub->Set_ ## field(value1);                                                                        \
   jude::Object::__PublishChanges(arrayTypes);   \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
         << "setting " #field " to same value should not notify again";                               \
                                                                                                      \
   sub->Set_ ## field(value2);                                                                        \
   jude::Object::__PublishChanges(arrayTypes);   \
   expectedNotificationCount++;                                                                       \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
         << "setting " #field " to different value should notify again";                              \
                                                                                                      \
   arrayTypes.Get_submsg_types().RemoveId(sub->Id());                                             \
} while(0)                                                                                            \

#define CHECK_NO_PUBLISH_ON_FIELD(field, value1, value2) do {                                         \
   auto sub = arrayTypes.Get_submsg_types().Add();                                                    \
   expectedNotificationCount = notificationCount;                                                     \
                                                                                                      \
   sub->Set_ ## field(value1);                                                                        \
   jude::Object::__PublishChanges(arrayTypes);   \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
         << "setting " #field " should not cause notification on publish when not subscibed to";      \
                                                                                                      \
   sub->Set_ ## field(value1);                                                                        \
   jude::Object::__PublishChanges(arrayTypes);   \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
         << "setting " #field " to same value should not notify again";                               \
                                                                                                      \
   sub->Set_ ## field(value2);                                                                        \
   jude::Object::__PublishChanges(arrayTypes);   \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
         << "setting " #field " to different value should notify again";                              \
                                                                                                      \
   arrayTypes.Get_submsg_types().RemoveId(sub->Id());                                             \
   jude::Object::__PublishChanges(arrayTypes);   \
   ASSERT_EQ(expectedNotificationCount, notificationCount)                                            \
      << "removing should notify when not subscribed to 'id'";                                        \
} while(0)                                                                                            \

#define CHECK_NO_CONST_PUBLISHES() do { \
   arrayTypes.Get_submsg_types().capacity();       \
   arrayTypes.Get_submsg_types().count();          \
   arrayTypes.Get_submsg_types().Find(1).has_value();   \
   arrayTypes.Get_submsg_types().Full();           \
   jude::Object::__PublishChanges(arrayTypes);   \
   ASSERT_EQ(0, notificationCount) << "const functions should not created notifications on publish"; \
} while (0)

#define VERIFY_RANGE_LOOP(msg, ...) do {    \
   jude_id_t ids[] = { 0, __VA_ARGS__ };  \
   size_t expectedCount = (sizeof(ids) / sizeof(ids[0])) - 1; \
   size_t count = 0;                        \
   for (const auto& sub : subArray)               \
   {                                        \
      count++;                              \
      ASSERT_EQ(ids[count], sub.Id()) << msg;  \
   }                                        \
   ASSERT_EQ(expectedCount, count);         \
} while (0)

#define ADD_OBJECTS_WITH_IDS(...)   do {    \
   jude_id_t ids[] = { __VA_ARGS__ };  \
   int count = (int)(sizeof(ids) / sizeof(ids[0])); \
   for (int i = 0; i < count; i++)               \
   {                                        \
      ASSERT_TRUE(subArray.Add(ids[i]).has_value()) << "Could not add object with id " << ids[i];  \
   }                                        \
} while (0)


TEST_F(ObjectArrayTests, range_for_loop_test)
{
   auto subArray = arrayTypes.Get_submsg_types();

   VERIFY_RANGE_LOOP("Empty array");
   ADD_OBJECTS_WITH_IDS(1,5,10);

   ASSERT_EQ(subArray.count(), 3);

   for (auto element : subArray)
   {
      switch (element.Id())
      {
         case 1:
         case 5:
         case 10:
         break;
         default:
         FAIL();
      }
   }
}

TEST_F(ObjectArrayTests, full_range_for_loop_test)
{
   auto subArray = arrayTypes.Get_submsg_types();

   VERIFY_RANGE_LOOP("Empty array");
   ADD_OBJECTS_WITH_IDS(1,5,10);
   VERIFY_RANGE_LOOP("added 3 objects 1,5,10", 1, 5, 10);
   ADD_OBJECTS_WITH_IDS(2,4,6);
   VERIFY_RANGE_LOOP("added 3 objects 2,4,6", 1, 5, 10, 2, 4, 6);
   subArray.RemoveId(10);
   VERIFY_RANGE_LOOP("Removed 10", 1, 5, 2, 4, 6);
   subArray.RemoveId(1);
   VERIFY_RANGE_LOOP("Removed 1", 5, 2, 4, 6);
   subArray.RemoveId(6);
   VERIFY_RANGE_LOOP("Removed 6", 5, 2, 4);
   subArray.Add(32);
   VERIFY_RANGE_LOOP("Added 32", 32, 5, 2, 4); // reuses empty first slot!
   subArray.Add(100);
   VERIFY_RANGE_LOOP("Added 32", 32, 5, 100, 2, 4); // reuses empty middle slot!
   subArray.clear();
   VERIFY_RANGE_LOOP("Emptied array");

}

TEST_F(ObjectArrayTests, range_for_loop_for_const)
{
   size_t count = 0;
   const auto subArray = arrayTypes.Get_submsg_types();
   const auto& const_copy = subArray;

   for (auto& sub : const_copy)
   {
      sub.Get_substuff1_or("Hi");
      count++;
   }
}