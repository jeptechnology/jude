#include <gtest/gtest.h>
#include <string.h>
#include "core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test.model.h"
#include "autogen/alltypes_test/TagsTestSubMessage.h"

#if defined(_WIN32) || defined(_WIN64)
# define strtok_r strtok_s
#endif

class BrowserTests : public JudeTestBase
{
public:
};

TEST_F(BrowserTests, test_init_optionals)
{
   // Given
   jude_browser_t browser = jude_browser_init(optionals_object, jude_user_Root);

   // Then
   ASSERT_TRUE(jude_browser_is_object(&browser));
   ASSERT_FALSE(jude_browser_is_array(&browser));
   ASSERT_FALSE(jude_browser_is_field(&browser));

   ASSERT_EQ(optionals_object, jude_browser_get_object(&browser));
   ASSERT_EQ(NULL, jude_browser_get_array(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_data(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, test_init_repeats)
{
   // Given
   jude_browser_t browser = jude_browser_init(repeats_object, jude_user_Root);

   // Then
   ASSERT_TRUE(jude_browser_is_object(&browser));
   ASSERT_FALSE(jude_browser_is_array(&browser));
   ASSERT_FALSE(jude_browser_is_field(&browser));
   ASSERT_TRUE(jude_browser_is_valid(&browser));
   ASSERT_EQ(jude_rest_OK, browser.code);

   ASSERT_EQ(repeats_object, jude_browser_get_object(&browser));
   ASSERT_EQ(NULL, jude_browser_get_array(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_data(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, test_browse_from_object_to_field)
{
   // Given
   jude_browser_t browser = jude_browser_init(optionals_object, jude_user_Root);
   const auto int32_field = jude_rtti_find_field(&AllOptionalTypes_rtti, "int32_type");

   // When
   auto result = jude_browser_follow_path(&browser, "int32_type", jude_permission_None);

   // Then
   ASSERT_TRUE(result);
   ASSERT_FALSE(jude_browser_is_object(&browser));
   ASSERT_FALSE(jude_browser_is_array(&browser));
   ASSERT_TRUE(jude_browser_is_field(&browser));
   ASSERT_TRUE(jude_browser_is_valid(&browser));
   ASSERT_EQ(jude_rest_OK, browser.code);

   ASSERT_EQ(NULL, jude_browser_get_object(&browser));
   ASSERT_EQ(NULL, jude_browser_get_array(&browser));
   ASSERT_EQ(&ptrOptionals.m_int32_type, jude_browser_get_field_data(&browser));
   ASSERT_EQ(int32_field, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, test_browse_from_object_to_array)
{
   // Given
   jude_browser_t browser = jude_browser_init(repeats_object, jude_user_Root);
   const auto int32_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "int32_type");

   // When
   auto result = jude_browser_follow_path(&browser, "int32_type", jude_permission_None);

   // Then
   ASSERT_TRUE(result);
   ASSERT_FALSE(jude_browser_is_object(&browser));
   ASSERT_TRUE(jude_browser_is_array(&browser));
   ASSERT_FALSE(jude_browser_is_field(&browser));
   ASSERT_TRUE(jude_browser_is_valid(&browser));
   ASSERT_EQ(jude_rest_OK, browser.code);

   ASSERT_EQ(NULL, jude_browser_get_object(&browser));
   ASSERT_EQ(int32_field, jude_browser_get_array(&browser)->current_field);
   ASSERT_EQ(NULL, jude_browser_get_field_data(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, test_browse_from_object_to_object)
{
   // Given
   jude_browser_t browser = jude_browser_init(optionals_object, jude_user_Root);

   // When
   auto result = jude_browser_follow_path(&browser, "submsg_type", jude_permission_None);

   // Then
   ASSERT_TRUE(result);
   ASSERT_TRUE(jude_browser_is_object(&browser));
   ASSERT_FALSE(jude_browser_is_array(&browser));
   ASSERT_FALSE(jude_browser_is_field(&browser));
   ASSERT_TRUE(jude_browser_is_valid(&browser));
   ASSERT_EQ(jude_rest_OK, browser.code);

   ASSERT_EQ((jude_object_t*)&ptrOptionals.m_submsg_type, jude_browser_get_object(&browser));
   ASSERT_EQ(NULL, jude_browser_get_array(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_data(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, test_browse_from_object_to_unknown)
{
   // Given
   jude_browser_t browser = jude_browser_init(repeats_object, jude_user_Root);

   // When
   auto result = jude_browser_follow_path(&browser, "unknown_field_name", jude_permission_None);

   // Then
   ASSERT_FALSE(result);
   ASSERT_FALSE(jude_browser_is_valid(&browser));
   ASSERT_EQ(jude_rest_Not_Found, browser.code);
}

static jude_browser_t* copy_of(jude_browser_t *browser)
{
   static jude_browser_t copy;
   memcpy(&copy, browser, sizeof(copy));
   return &copy;
}

TEST_F(BrowserTests, test_browse_from_array_to_object)
{
   // Given
   jude_browser_t browser = jude_browser_init(repeats_object, jude_user_Root);

   // When
   ASSERT_TRUE(jude_browser_follow_path(&browser, "submsg_type", jude_permission_None));

   // Then... first attempt - submessage array is empty
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "0", jude_permission_None));
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "5", jude_permission_None));

   // When... object array has some data - but not id of "5"
   ptrRepeats.m_submsg_type_count = 2;
   jude_filter_set_touched(ptrRepeats.__mask, RepeatedSubMessage->index, true);
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "0", jude_permission_None));
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "5", jude_permission_None));

   // When... object array has id of "5" but "id" field not marked as set...
   ptrRepeats.m_submsg_type[0].m_id = 4;
   ptrRepeats.m_submsg_type[1].m_id = 5;
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "0", jude_permission_None));
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "5", jude_permission_None));

   // When... finally, we have an object with id of "1" and "id" field marked as set...
   jude_filter_set_touched(ptrRepeats.m_submsg_type[0].__mask, 0, true);
   jude_filter_set_touched(ptrRepeats.m_submsg_type[1].__mask, 0, true);

   // Then
   ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "0", jude_permission_None));
   ASSERT_TRUE(jude_browser_follow_path(&browser, "5", jude_permission_None));

   ASSERT_TRUE(jude_browser_is_object(&browser));
   ASSERT_FALSE(jude_browser_is_array(&browser));
   ASSERT_FALSE(jude_browser_is_field(&browser));

   ASSERT_EQ((jude_object_t*)&ptrRepeats.m_submsg_type[1], jude_browser_get_object(&browser));
   ASSERT_EQ(NULL, jude_browser_get_array(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_data(&browser));
   ASSERT_EQ(NULL, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, test_browse_from_array_to_field)
{
	// Given
	jude_browser_t browser = jude_browser_init(repeats_object, jude_user_Root);
	const auto int16_field = jude_rtti_find_field(&AllRepeatedTypes_rtti, "int16_type");

	// When
	ASSERT_TRUE(jude_browser_follow_path(&browser, "int16_type", jude_permission_None));

	// Then... first attempt - field array is empty
	ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "5", jude_permission_None));

	// When... field array has some data - but count is only 3
	ptrRepeats.m_int16_type_count = 3;
	jude_filter_set_touched(ptrRepeats.__mask, int16_field->index, true);
	ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "5", jude_permission_None));

	// When... field array has count of "5" but we remember that index is zero-based
	ptrRepeats.m_int16_type_count = 5;
	ASSERT_FALSE(jude_browser_follow_path(copy_of(&browser), "5", jude_permission_None));

	// When... finally, we have count of 6...
	ptrRepeats.m_int16_type_count = 6;

	// Then
	ASSERT_TRUE(jude_browser_follow_path(&browser, "5", jude_permission_None));

	ASSERT_FALSE(jude_browser_is_object(&browser));
	ASSERT_FALSE(jude_browser_is_array(&browser));
	ASSERT_TRUE(jude_browser_is_field(&browser));

	ASSERT_EQ(NULL, jude_browser_get_object(&browser));
	ASSERT_EQ(NULL, jude_browser_get_array(&browser));
	ASSERT_EQ(&ptrRepeats.m_int16_type[5], jude_browser_get_field_data(&browser));
	ASSERT_EQ(int16_field, jude_browser_get_field_type(&browser));
}

TEST_F(BrowserTests, browse_from_array_to_object_via_search)
{
	// Given
	jude_browser_t browser = jude_browser_init(repeats_object, jude_user_Root);
	const auto submsg_field = jude_rtti_find_field(&AllOptionalTypes_rtti, "submsg_type");
	const auto substuff1_field = jude_rtti_find_field(&SubMessage_rtti, "substuff1");
	ptrRepeats.m_submsg_type_count = 2;
	jude_filter_set_touched(ptrRepeats.__mask, submsg_field->index, true);
	ptrRepeats.m_submsg_type[0].m_id = 2;
	ptrRepeats.m_submsg_type[1].m_id = 5;
	strncpy(ptrRepeats.m_submsg_type[0].m_substuff1, "John", substuff1_field->data_size);
	strncpy(ptrRepeats.m_submsg_type[1].m_substuff1, "Peter", substuff1_field->data_size);
	jude_filter_set_touched(ptrRepeats.m_submsg_type[0].__mask, 0, true);
	jude_filter_set_touched(ptrRepeats.m_submsg_type[1].__mask, 0, true);
	jude_filter_set_touched(ptrRepeats.m_submsg_type[0].__mask, substuff1_field->index, true);
	jude_filter_set_touched(ptrRepeats.m_submsg_type[1].__mask, substuff1_field->index, true);
	ASSERT_TRUE(jude_browser_follow_path(&browser, "submsg_type", jude_permission_None));

	// Then
	ASSERT_FALSE(jude_browser_follow_path(&browser, "*substuff1=David", jude_permission_None)); // can't find David
   ASSERT_FALSE(jude_browser_follow_path(&browser, "*Name=Peter", jude_permission_None));  // shouldn't find Peter as"Name"
   ASSERT_TRUE (jude_browser_follow_path(&browser, "*substuff1=Peter", jude_permission_None));  // should find Peter

	ASSERT_TRUE(jude_browser_is_object(&browser));
	ASSERT_FALSE(jude_browser_is_array(&browser));
	ASSERT_FALSE(jude_browser_is_field(&browser));

	ASSERT_EQ((jude_object_t*)&ptrRepeats.m_submsg_type[1], jude_browser_get_object(&browser));
	ASSERT_EQ(NULL, jude_browser_get_array(&browser));
	ASSERT_EQ(NULL, jude_browser_get_field_data(&browser));
	ASSERT_EQ(NULL, jude_browser_get_field_type(&browser));
}

static jude_browser_t AttemptBrowse(jude::Object& resource, char *path, jude_user_t access_level, jude_permission_t readOrWrite)
{
   jude_browser_t browser = jude_browser_init(resource.RawData(), access_level);
   char* tmp;
   char* token = strtok_r(path, "/", &tmp);
   while (token != nullptr)
   {
      if (!jude_browser_follow_path(&browser, token, readOrWrite))
      {
         return browser;
      }
      token = strtok_r(nullptr, "/", &tmp);
   }
   return browser;
}

#define AssertBrowseIsOK(resource, path, access_level, readOrWrite) \
   do { \
      char uri[] = path; \
      auto browser = AttemptBrowse(resource, uri, access_level, readOrWrite); \
      ASSERT_TRUE(jude_browser_is_valid(&browser)); \
   } while(0)

#define AssertBrowseFail(resource, path, access_level, readOrWrite) \
   do { \
      char uri[] = path; \
      auto browser = AttemptBrowse(resource, uri, access_level, readOrWrite); \
      ASSERT_FALSE(jude_browser_is_valid(&browser)); \
   } while(0)

TEST_F(BrowserTests, public_read_browse_access_rights)
{
   auto resource = jude::TagsTestSubMessage::New();

   // public read browsing always fails on private root...
   AssertBrowseFail(resource, "privateConfig",               jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateConfig/privateConfig", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateConfig/publicConfig",  jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateConfig/privateStatus", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateConfig/publicStatus",  jude_user_Public, jude_permission_Read);

   AssertBrowseFail(resource, "privateStatus", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateStatus/privateConfig", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateStatus/publicConfig", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateStatus/privateStatus", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "privateStatus/publicStatus", jude_user_Public, jude_permission_Read);

   AssertBrowseIsOK(resource, "publicConfig",                jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "publicConfig/privateConfig",  jude_user_Public, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicConfig/publicConfig",   jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "publicConfig/privateStatus",  jude_user_Public, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicConfig/publicStatus",   jude_user_Public, jude_permission_Read);

   AssertBrowseIsOK(resource, "publicStatus", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "publicStatus/privateConfig", jude_user_Public, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicStatus/publicConfig", jude_user_Public, jude_permission_Read);
   AssertBrowseFail(resource, "publicStatus/privateStatus", jude_user_Public, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicStatus/publicStatus", jude_user_Public, jude_permission_Read);
}

TEST_F(BrowserTests, public_write_browse_access_rights)
{
   auto resource = jude::TagsTestSubMessage::New();

   AssertBrowseFail(resource, "privateConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateConfig/privateConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateConfig/publicConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateConfig/privateStatus", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateConfig/publicStatus", jude_user_Public, jude_permission_Write);

   AssertBrowseFail(resource, "privateStatus", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateStatus/privateConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateStatus/publicConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateStatus/privateStatus", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "privateStatus/publicStatus", jude_user_Public, jude_permission_Write);

   AssertBrowseIsOK(resource, "publicConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicConfig/privateConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicConfig/publicConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicConfig/privateStatus", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicConfig/publicStatus", jude_user_Public, jude_permission_Write);

   AssertBrowseFail(resource, "publicStatus", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicStatus/privateConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicStatus/publicConfig", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicStatus/privateStatus", jude_user_Public, jude_permission_Write);
   AssertBrowseFail(resource, "publicStatus/publicStatus", jude_user_Public, jude_permission_Write);
}
TEST_F(BrowserTests, private_read_browse_access_rights)
{
   auto resource = jude::TagsTestSubMessage::New();

   // private read browsing should be able to read *everything*!!
   AssertBrowseIsOK(resource, "privateConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateConfig/privateConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateConfig/publicConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateConfig/privateStatus", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateConfig/publicStatus", jude_user_Root, jude_permission_Read);

   AssertBrowseIsOK(resource, "privateStatus", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateStatus/privateConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateStatus/publicConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateStatus/privateStatus", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "privateStatus/publicStatus", jude_user_Root, jude_permission_Read);

   AssertBrowseIsOK(resource, "publicConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicConfig/privateConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicConfig/publicConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicConfig/privateStatus", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicConfig/publicStatus", jude_user_Root, jude_permission_Read);

   AssertBrowseIsOK(resource, "publicStatus", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicStatus/privateConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicStatus/publicConfig", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicStatus/privateStatus", jude_user_Root, jude_permission_Read);
   AssertBrowseIsOK(resource, "publicStatus/publicStatus", jude_user_Root, jude_permission_Read);
}

TEST_F(BrowserTests, private_write_browse_access_rights)
{
   auto resource = jude::TagsTestSubMessage::New();

   // A private browser can write to *everything*
   AssertBrowseIsOK(resource, "privateConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateConfig/privateConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateConfig/publicConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateConfig/privateStatus", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateConfig/publicStatus", jude_user_Root, jude_permission_Write);

   AssertBrowseIsOK(resource, "privateStatus", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateStatus/privateConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateStatus/publicConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateStatus/privateStatus", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "privateStatus/publicStatus", jude_user_Root, jude_permission_Write);

   AssertBrowseIsOK(resource, "publicConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicConfig/privateConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicConfig/publicConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicConfig/privateStatus", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicConfig/publicStatus", jude_user_Root, jude_permission_Write);

   AssertBrowseIsOK(resource, "publicStatus", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicStatus/privateConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicStatus/publicConfig", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicStatus/privateStatus", jude_user_Root, jude_permission_Write);
   AssertBrowseIsOK(resource, "publicStatus/publicStatus", jude_user_Root, jude_permission_Write);
}
