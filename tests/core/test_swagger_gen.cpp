#include <gtest/gtest.h>
#include <algorithm>
#include <ctype.h>

#include "core/test_base.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "autogen/alltypes_test/ActionTest.h"
#include "../database/FullSwaggerText.h"

using namespace std;
using namespace jude;

class SwaggerTests : public JudeTestBase
{
public:
   void CheckSchemasAre(DatabaseEntry& dbEntry, jude_user_t userLevel, std::string expected)
   {
      auto actual = dbEntry.GetSwaggerReadSchema(userLevel);
      ASSERT_STREQ(actual.c_str(), expected.c_str());
   }

};

TEST_F(SwaggerTests, Individual_swagger_paths)
{
   jude::Resource<SubMessage> sub("Hello", jude_user_Admin);

   std::stringstream output;
   sub.OutputAllSwaggerPaths(output, "", jude_user_Admin);
   
   const char* expectedOutput = R"(  /Hello/:
    get:
      summary: Get the Hello resource data
      tags:
        - /Hello
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the Hello resource 
      tags:
        - /Hello
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";


   ASSERT_STREQ(expectedOutput, output.str().c_str());
}

TEST_F(SwaggerTests, Individual_swagger_paths_with_actions)
{
   jude::Resource<ActionTest> sub("Actions", jude_user_Admin);

   std::stringstream output;
   sub.OutputAllSwaggerPaths(output, "", jude_user_Admin);

   const char* expectedOutput = R"(  /Actions/:
    get:
      summary: Get the Actions resource data
      tags:
        - /Actions
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the Actions resource 
      tags:
        - /Actions
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/actionOnBool:
    patch:
      summary: Invoke the action actionOnBool() on the Actions resource
      tags:
        - /Actions
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: boolean

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/actionOnInteger:
    patch:
      summary: Invoke the action actionOnInteger() on the Actions resource
      tags:
        - /Actions
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: integer

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/actionOnString:
    patch:
      summary: Invoke the action actionOnString() on the Actions resource
      tags:
        - /Actions
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: string
              maxLength: 31

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/actionOnObject:
    patch:
      summary: Invoke the action actionOnObject() on the Actions resource
      tags:
        - /Actions
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              $ref: '#/components/schemas/AllOptionalTypes_Schema'

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";


   ASSERT_STREQ(expectedOutput, output.str().c_str());
}

TEST_F(SwaggerTests, Collection_swagger_paths)
{
   jude::Collection<SubMessage> subs("Hello", 16, jude_user_Admin);

   std::stringstream output;
   subs.OutputAllSwaggerPaths(output, "", jude_user_Admin);

   const char* expectedOutput = R"(  /Hello/:
    get:
      summary: Get all entries in the Hello collection
      tags:
        - /Hello
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 type: array
                 items:
                   $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    post:
      summary: Create a new entry in the Hello collection
      tags:
        - /Hello
      requestBody:
        description: Object to add to collection
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '201':
           description: SubMessage created
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized

  /Hello/{id}:
    get:
      summary: Get entry in the Hello collection with given id
      tags:
        - /Hello
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the entry in the Hello collection with given id
      tags:
        - /Hello
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

    delete:
      summary: Delete the entry in the Hello collection with given id
      tags:
        - /Hello
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '204':
           description: Object deleted
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

   ASSERT_STREQ(expectedOutput, output.str().c_str());
}

TEST_F(SwaggerTests, Collection_swagger_paths_with_actions)
{
   jude::Collection<ActionTest> sub("Actions", 16, jude_user_Admin);

   std::stringstream output;
   sub.OutputAllSwaggerPaths(output, "", jude_user_Admin);

   const char* expectedOutput = R"(  /Actions/:
    get:
      summary: Get all entries in the Actions collection
      tags:
        - /Actions
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 type: array
                 items:
                   $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    post:
      summary: Create a new entry in the Actions collection
      tags:
        - /Actions
      requestBody:
        description: Object to add to collection
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '201':
           description: ActionTest created
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized

  /Actions/{id}:
    get:
      summary: Get entry in the Actions collection with given id
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the entry in the Actions collection with given id
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

    delete:
      summary: Delete the entry in the Actions collection with given id
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '204':
           description: Object deleted
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/{id}/actionOnBool:
    patch:
      summary: Invoke the action actionOnBool() on the Actions resource
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: boolean

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/{id}/actionOnInteger:
    patch:
      summary: Invoke the action actionOnInteger() on the Actions resource
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: integer

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/{id}/actionOnString:
    patch:
      summary: Invoke the action actionOnString() on the Actions resource
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: string
              maxLength: 31

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Actions/{id}/actionOnObject:
    patch:
      summary: Invoke the action actionOnObject() on the Actions resource
      tags:
        - /Actions
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              $ref: '#/components/schemas/AllOptionalTypes_Schema'

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";


   ASSERT_STREQ(expectedOutput, output.str().c_str());
}

TEST_F(SwaggerTests, Database_swagger_paths)
{
   TestDatabase db("Top");
   jude::Resource<ActionTest> ind("Single", jude_user_Admin);
   jude::Collection<SubMessage> col("Collection", 16, jude_user_Admin);
   db.AddToDB(ind);
   db.AddToDB(col);

   TestDatabase sub("SubDB");
   jude::Resource<ActionTest> indSub("SubSingle", jude_user_Admin);
   sub.AddToDB(indSub);
   db.AddToDB(sub);

   std::stringstream output;
   db.OutputAllSwaggerPaths(output, "", jude_user_Admin);

   const char* expectedOutput = R"(  /Top/:
    get:
      summary: Get the Top resource data
      tags:
        - Top
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/Top_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/Collection/:
    get:
      summary: Get all entries in the Collection collection
      tags:
        - /Top/Collection
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 type: array
                 items:
                   $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    post:
      summary: Create a new entry in the Collection collection
      tags:
        - /Top/Collection
      requestBody:
        description: Object to add to collection
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '201':
           description: SubMessage created
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized

  /Top/Collection/{id}:
    get:
      summary: Get entry in the Collection collection with given id
      tags:
        - /Top/Collection
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the entry in the Collection collection with given id
      tags:
        - /Top/Collection
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

    delete:
      summary: Delete the entry in the Collection collection with given id
      tags:
        - /Top/Collection
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '204':
           description: Object deleted
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/Single/:
    get:
      summary: Get the Single resource data
      tags:
        - /Top/Single
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the Single resource 
      tags:
        - /Top/Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/Single/actionOnBool:
    patch:
      summary: Invoke the action actionOnBool() on the Single resource
      tags:
        - /Top/Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: boolean

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/Single/actionOnInteger:
    patch:
      summary: Invoke the action actionOnInteger() on the Single resource
      tags:
        - /Top/Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: integer

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/Single/actionOnString:
    patch:
      summary: Invoke the action actionOnString() on the Single resource
      tags:
        - /Top/Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: string
              maxLength: 31

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/Single/actionOnObject:
    patch:
      summary: Invoke the action actionOnObject() on the Single resource
      tags:
        - /Top/Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              $ref: '#/components/schemas/AllOptionalTypes_Schema'

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/SubDB/:
    get:
      summary: Get the SubDB resource data
      tags:
        - SubDB
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubDB_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/SubDB/SubSingle/:
    get:
      summary: Get the SubSingle resource data
      tags:
        - /Top/SubDB/SubSingle
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the SubSingle resource 
      tags:
        - /Top/SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/SubDB/SubSingle/actionOnBool:
    patch:
      summary: Invoke the action actionOnBool() on the SubSingle resource
      tags:
        - /Top/SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: boolean

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/SubDB/SubSingle/actionOnInteger:
    patch:
      summary: Invoke the action actionOnInteger() on the SubSingle resource
      tags:
        - /Top/SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: integer

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/SubDB/SubSingle/actionOnString:
    patch:
      summary: Invoke the action actionOnString() on the SubSingle resource
      tags:
        - /Top/SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: string
              maxLength: 31

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Top/SubDB/SubSingle/actionOnObject:
    patch:
      summary: Invoke the action actionOnObject() on the SubSingle resource
      tags:
        - /Top/SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              $ref: '#/components/schemas/AllOptionalTypes_Schema'

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

   ASSERT_STREQ(expectedOutput, output.str().c_str());
}

TEST_F(SwaggerTests, Swagger_full_generation)
{
   TestDatabase db(""); // top level database does not have to have a name!
   jude::Resource<ActionTest> ind("Single", jude_user_Admin);
   jude::Collection<SubMessage> col("Collection", 16, jude_user_Admin);

   Options::SerialiseCollectionAsObjectMap = false;

   db.AddToDB(ind);
   db.AddToDB(col);

   TestDatabase sub("SubDB");
   jude::Resource<ActionTest> indSub("SubSingle", jude_user_Admin);
   sub.AddToDB(indSub);
   db.AddToDB(sub);

   std::stringstream output;
   db.GenerateYAMLforSwaggerOAS3(output, jude_user_Admin);

   string expectedOutput = ExpectedFullSwaggerDefinitionHeader;
   expectedOutput += ExpectedFullSwaggerDefinitionComponents;

   Options::SerialiseCollectionAsObjectMap = true;

   ASSERT_STREQ(expectedOutput.c_str(), output.str().c_str());
}

TEST_F(SwaggerTests, Swagger_tagstest_generation)
{
   TestDatabase db(""); // top level database does not have to have a name!
   jude::Resource<TagsTest> ind("Single", jude_user_Admin);
   db.AddToDB(ind);

   std::stringstream output;
   db.GenerateYAMLforSwaggerOAS3(output, jude_user_Admin);

   string expectedOutput = TagsTestSwagger;

   ASSERT_STREQ(expectedOutput.c_str(), output.str().c_str());
}
