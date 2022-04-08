#include <gmock/gmock.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Resource.h"
#include "jude/database/Database.h"
#include "jude/database/Collection.h"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace jude;

using testing::ElementsAre;
using testing::UnorderedElementsAre;
using testing::IsEmpty;
using testing::AllOf;
using testing::Contains;
using testing::Ne;
using testing::Each;
using testing::StartsWith;
using testing::EndsWith;

class ObjectSearchTests : public JudeTestBase
{
public:
   jude::AllOptionalTypes resource;
   jude::AllRepeatedTypes arrays;
   
   ObjectSearchTests()
      : resource(jude::AllOptionalTypes::New())
      , arrays(jude::AllRepeatedTypes::New())
   {}
};

TEST_F(ObjectSearchTests, finds_nothing_on_empty_string)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "", 8, jude_user_Admin), IsEmpty());
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "", 8, jude_user_Admin), IsEmpty());
}

TEST_F(ObjectSearchTests, finds_all_fields_on_single_slash)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/", 16, jude_user_Admin), UnorderedElementsAre(
      "/id"
      , "/int8_type"
      , "/int16_type"
      , "/int32_type"
      , "/int64_type"
      , "/uint8_type"
      , "/uint16_type"
      , "/uint32_type"
      , "/uint64_type"
      , "/bool_type"
      , "/string_type"
      , "/bytes_type"
      , "/submsg_type"
      , "/enum_type"
      , "/bitmask_type"));
}

TEST_F(ObjectSearchTests, adds_slash_to_all_array_fields)
{
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/", 16, jude_user_Admin), UnorderedElementsAre(
      "/id"
      , "/int8_type"
      , "/int16_type"
      , "/int32_type"
      , "/int64_type"
      , "/uint8_type"
      , "/uint16_type"
      , "/uint32_type"
      , "/uint64_type"
      , "/bool_type"
      , "/string_type"
      , "/bytes_type"
      , "/submsg_type"
      , "/enum_type"
      , "/bitmask_type"));
}

TEST_F(ObjectSearchTests, finds_matching_prefixes_with_slash)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/i", 16, jude_user_Admin), UnorderedElementsAre(
      "/id"
      , "/int8_type"
      , "/int16_type"
      , "/int32_type"
      , "/int64_type"));

   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/b", 16, jude_user_Admin), UnorderedElementsAre(
      "/bool_type"
      , "/bytes_type"
      , "/bitmask_type"));

}

TEST_F(ObjectSearchTests, finds_nothing_without_slash)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "i", 16, jude_user_Admin), IsEmpty());
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "b", 16, jude_user_Admin), IsEmpty());
}

TEST_F(ObjectSearchTests, finds_unique_match)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/bool", 16, jude_user_Admin), ElementsAre("/bool_type"));
}

TEST_F(ObjectSearchTests, finds_exact_match)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/bool_type", 16, jude_user_Admin), ElementsAre("/bool_type"));
}

TEST_F(ObjectSearchTests, finds_one_subresource)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/submsg_type", 16, jude_user_Admin), ElementsAre("/submsg_type"));
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/submsg_", 16, jude_user_Admin), ElementsAre("/submsg_type"));
}

TEST_F(ObjectSearchTests, finds_inside_subresource)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/submsg_type/", 16, jude_user_Admin), UnorderedElementsAre(
       "/submsg_type/id"
     , "/submsg_type/substuff1"
     , "/submsg_type/substuff2"
     , "/submsg_type/substuff3"));
}

TEST_F(ObjectSearchTests, finds_inside_subresource_unique_match)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/submsg_type/i", 16, jude_user_Admin), ElementsAre("/submsg_type/id"));
}

TEST_F(ObjectSearchTests, finds_inside_subresource_exact_match)
{
   EXPECT_THAT(resource.SearchForPath(CRUD::READ, "/submsg_type/substuff2", 16, jude_user_Admin), ElementsAre("/submsg_type/substuff2"));
}

TEST_F(ObjectSearchTests, exact_array_search)
{
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/string_type", 16, jude_user_Admin), ElementsAre("/string_type"));
}

TEST_F(ObjectSearchTests, finds_nothing_inside_empty_array)
{
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/string_type", 16, jude_user_Admin), ElementsAre("/string_type"));
   arrays.Get_string_types().Add("one");

}

TEST_F(ObjectSearchTests, completes_array_indeces_for_atomic_array)
{
   arrays.Get_string_types().Add("one");
   arrays.Get_string_types().Add("two");
   arrays.Get_string_types().Add("three");

   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/string_type/", 16, jude_user_Admin), UnorderedElementsAre("/string_type/0", "/string_type/1", "/string_type/2"));
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/string_type/1", 16, jude_user_Admin), UnorderedElementsAre("/string_type/1"));
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/string_type/6", 16, jude_user_Admin), IsEmpty());
}

TEST_F(ObjectSearchTests, completes_array_ids_for_object_array)
{
   arrays.Get_submsg_types().Add(100);
   arrays.Get_submsg_types().Add(120);
   arrays.Get_submsg_types().Add(123);

   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/submsg_type/", 16, jude_user_Admin), UnorderedElementsAre("/submsg_type/100", "/submsg_type/120", "/submsg_type/123"));
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/submsg_type/12", 16, jude_user_Admin), UnorderedElementsAre("/submsg_type/120", "/submsg_type/123"));
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/submsg_type/5", 16, jude_user_Admin), IsEmpty());
}

TEST_F(ObjectSearchTests, completes_fields_inside_subresource_array)
{
   arrays.Get_submsg_types().Add(123);

   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/submsg_type/123", 16, jude_user_Admin), UnorderedElementsAre("/submsg_type/123"));
   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/submsg_type/123/", 16, jude_user_Admin), UnorderedElementsAre(
        "/submsg_type/123/id"
      , "/submsg_type/123/substuff1"
      , "/submsg_type/123/substuff2"
      , "/submsg_type/123/substuff3"));

   EXPECT_THAT(arrays.SearchForPath(CRUD::READ, "/submsg_type/123/i", 16, jude_user_Admin), UnorderedElementsAre("/submsg_type/123/id"));
}
