#include <gmock/gmock.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "autogen/alltypes_test/AllDB.h"
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

class DatabaseSearchTests : public JudeTestBase
{
public:
   PopulatedDB db;
};

TEST_F(DatabaseSearchTests, finds_nothing_on_empty_string)
{
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "", 8, jude_user_Root), IsEmpty());
   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "", 8, jude_user_Root), IsEmpty());
   EXPECT_THAT(db.resource1.SearchForPath(CRUD::READ, "", 8, jude_user_Root), IsEmpty());
}

TEST_F(DatabaseSearchTests, finds_all_entries_in_db_on_single_slash)
{
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/", 16, jude_user_Root), UnorderedElementsAre(
        "/collection1"
      , "/collection2"
      , "/resource1"
      , "/resource2"
      , "/subDB"));
}

TEST_F(DatabaseSearchTests, finds_matching_entries_in_db_on_prefix)
{
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/coll", 16, jude_user_Root), UnorderedElementsAre("/collection1", "/collection2"));
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/subDB", 16, jude_user_Root), UnorderedElementsAre("/subDB"));
}

TEST_F(DatabaseSearchTests, finds_all_fields_in_object_on_single_slash)
{
   // through individual
   EXPECT_THAT(db.resource1.SearchForPath(CRUD::READ, "/", 16, jude_user_Root), UnorderedElementsAre(
      "/id", 
      "/substuff1", 
      "/substuff2", 
      "/substuff3"));

   // through database
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/resource1/", 16, jude_user_Root), UnorderedElementsAre(
      "/resource1/id",
      "/resource1/substuff1",
      "/resource1/substuff2",
      "/resource1/substuff3"));
}

TEST_F(DatabaseSearchTests, finds_nothing_without_slash)
{
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "r", 16, jude_user_Root), IsEmpty());
   EXPECT_THAT(db.resource1.SearchForPath(CRUD::READ, "s", 16, jude_user_Root), IsEmpty());
   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "s", 16, jude_user_Root), IsEmpty());
}

TEST_F(DatabaseSearchTests, exact_collection_search)
{
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection1", 16, jude_user_Root), ElementsAre("/collection1"));
   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "", 16, jude_user_Root), IsEmpty());
}

TEST_F(DatabaseSearchTests, finds_nothing_inside_empty_collection)
{
   db.collection1.clear();
   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "/", 16, jude_user_Root), IsEmpty());
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection1/", 16, jude_user_Root), IsEmpty());
}

TEST_F(DatabaseSearchTests, completes_array_ids_for_object_array)
{
   db.collection1.clear();
   db.collection1.Post(100);
   db.collection1.Post(120);
   db.collection1.Post(123);

   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection1/", 16, jude_user_Root), UnorderedElementsAre("/collection1/100", "/collection1/120", "/collection1/123"));
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection1/12", 16, jude_user_Root), UnorderedElementsAre("/collection1/120", "/collection1/123"));
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection1/5", 16, jude_user_Root), IsEmpty());

   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "/", 16, jude_user_Root), UnorderedElementsAre("/100", "/120", "/123"));
   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "/12", 16, jude_user_Root), UnorderedElementsAre("/120", "/123"));
   EXPECT_THAT(db.collection1.SearchForPath(CRUD::READ, "/5", 16, jude_user_Root), IsEmpty());
}

TEST_F(DatabaseSearchTests, completes_fields_inside_subresource_array)
{
   db.collection2.clear();
   db.collection2.Post(123);

   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection2/123", 16, jude_user_Root), UnorderedElementsAre("/collection2/123"));
   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection2/123/", 16, jude_user_Root), UnorderedElementsAre(
        "/collection2/123/id"
      , "/collection2/123/substuff1"
      , "/collection2/123/substuff2"
      , "/collection2/123/substuff3"));

   EXPECT_THAT(db.SearchForPath(CRUD::READ, "/collection2/123/i", 16, jude_user_Root), UnorderedElementsAre("/collection2/123/id"));
}

TEST_F(DatabaseSearchTests, completes_enum_fields)
{
   jude::AllDB alldb;

   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/alltypes/enum_type", 16, jude_user_Root), UnorderedElementsAre("/alltypes/enum_type"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/alltypes/enum_type ", 16, jude_user_Root), UnorderedElementsAre(
        "/alltypes/enum_type First"
      , "/alltypes/enum_type Second"
      , "/alltypes/enum_type Zero"
      , "/alltypes/enum_type Truth"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/alltypes/enum_type F", 16, jude_user_Root), UnorderedElementsAre("/alltypes/enum_type First"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/alltypes/enum_type Zer", 16, jude_user_Root), UnorderedElementsAre("/alltypes/enum_type Zero"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/alltypes/enum_type J", 16, jude_user_Root), IsEmpty());
}

TEST_F(DatabaseSearchTests, completes_enum_fields_in_collection)
{
   jude::AllDB alldb;
   alldb.list.Post(20);

   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/list/20/enum_type", 16, jude_user_Root), UnorderedElementsAre("/list/20/enum_type"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/list/20/enum_type ", 16, jude_user_Root), UnorderedElementsAre(
        "/list/20/enum_type First"
      , "/list/20/enum_type Second"
      , "/list/20/enum_type Zero"
      , "/list/20/enum_type Truth"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/list/20/enum_type F", 16, jude_user_Root), UnorderedElementsAre("/list/20/enum_type First"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/list/20/enum_type Zer", 16, jude_user_Root), UnorderedElementsAre("/list/20/enum_type Zero"));
   EXPECT_THAT(alldb.SearchForPath(CRUD::READ, "/list/20/enum_type J", 16, jude_user_Root), IsEmpty());
}
