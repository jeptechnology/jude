#include <gtest/gtest.h>
#include <jude/restapi/jude_rest_api.h>
#include <algorithm>
#include <ctype.h>

#include <jude/core/cpp/Stream.h>
#include "core/test_base.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

using namespace std;

class RestApiPostTests : public JudeTestBase
{
public:
   jude::AllOptionalTypes singleTypes = jude::AllOptionalTypes::New();
   jude::AllRepeatedTypes arrayTypes = jude::AllRepeatedTypes::New();

   jude_user_t ADMIN = jude_user_Admin;
   jude_user_t PUBLIC = jude_user_Public;

   jude_restapi_code_t OK = jude_rest_OK;
   jude_restapi_code_t Created = jude_rest_Created;
   jude_restapi_code_t No_Content = jude_rest_No_Content;
   jude_restapi_code_t Bad_Request = jude_rest_Bad_Request;
   jude_restapi_code_t Unauthorized = jude_rest_Unauthorized;
   jude_restapi_code_t Forbidden = jude_rest_Forbidden;
   jude_restapi_code_t Not_Found = jude_rest_Not_Found;
   jude_restapi_code_t Method_Not_Allowed = jude_rest_Method_Not_Allowed;
   jude_restapi_code_t Conflict = jude_rest_Conflict;
   jude_restapi_code_t Internal_Server_Error = jude_rest_Internal_Server_Error;

   std::string lastOutput;

   string& StripSpaces(string& json)
   {
      json.erase(remove_if(json.begin(), json.end(), ::isspace), json.end());
      return json;
   }

   void Verify_Post(jude_user_t user, const char* path, const char *inputJSON, jude_restapi_code_t expected_code, jude_id_t expected_id = JUDE_AUTO_ID)
   {
      stringstream mis(inputJSON);
      auto result = singleTypes.RestPost(path, mis, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "POST " << path << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
      if (expected_id != JUDE_AUTO_ID)
      {
         ASSERT_EQ(expected_id, result.GetCreatedObjectId()) << "new object / element from POST does not have the value I expected";
      }
   }

   void Verify_Array_Post(jude_user_t user, const char* path, const char* inputJSON, jude_restapi_code_t expected_code, jude_id_t expected_id = JUDE_AUTO_ID)
   {
      stringstream mis(inputJSON);
      auto result = arrayTypes.RestPost(path, mis, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "POST " << path << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
      if (expected_id != JUDE_AUTO_ID)
      {
         ASSERT_EQ(expected_id, result.GetCreatedObjectId()) << "new object / element from POST does not have the value I expected";
      }
   }

   void Verify_Get(jude_user_t user,
      const char* path,
      jude_restapi_code_t expected_code,
      const char* expected_result = nullptr)
   {
      stringstream mos;
      auto result = singleTypes.RestGet(path, mos, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "GET " << path << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
      if (expected_result)
      {
         EXPECT_STREQ(expected_result, mos.str().c_str()) << "GET " << path << " resulted in unexpected response";
      }

      lastOutput = mos.str();
   }

   void Verify_Array_Get(jude_user_t user,
      const char* path,
      jude_restapi_code_t expected_code,
      const char* expected_result = nullptr)
   {
      stringstream mos;
      auto result = arrayTypes.RestGet(path, mos, user);

      ASSERT_EQ(result.GetCode(), expected_code) << "GET " << path << " resulted in unexpected code: " << result.GetCode();
      if (expected_result)
      {
         string expected = expected_result;
         StripSpaces(expected);
         EXPECT_STREQ(expected.c_str(), mos.str().c_str()) << "GET " << path << " resulted in unexpected response";
      }

      lastOutput = mos.str();
   }

};

TEST_F(RestApiPostTests, post_bad_path_returns_404)
{
   Verify_Post(ADMIN, "/does_not_exist", "{}", Not_Found);
   Verify_Array_Post(ADMIN, "/does_not_exist", "{}", Not_Found);
}

TEST_F(RestApiPostTests, post_root_not_allowed)
{
   singleTypes
      .Set_bool_type(true)
      .Set_int16_type(123)
      .Set_string_type("Hello");

   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Post(ADMIN, "/", "", Method_Not_Allowed);
   Verify_Post(ADMIN, "", "", Method_Not_Allowed);
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
}

TEST_F(RestApiPostTests, post_individual_field_not_allowed)
{
   singleTypes
      .Set_bool_type(true)
      .Set_int16_type(123)
      .Set_string_type("Hello");

   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   
   Verify_Post(ADMIN, "/bool_type", "false", Method_Not_Allowed);
   Verify_Post(ADMIN, "/string_type/", R"("World")", Method_Not_Allowed);
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Post(ADMIN, "/int16_type", "456", Method_Not_Allowed);
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
}

TEST_F(RestApiPostTests, post_sub_object_individual_fields_not_allowed)
{
   auto sub = singleTypes.Get_submsg_type();
   sub.Set_substuff1("Hello").Set_substuff2(32).Set_substuff3(true);

   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");
   
   Verify_Post(ADMIN, "/submsg_type/", R"({"substuff2":55})", Method_Not_Allowed);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");

   Verify_Post(ADMIN, "/submsg_type/", R"({"substuff3":false,"substuff1":"World"})", Method_Not_Allowed);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");

   Verify_Post(ADMIN, "/submsg_type/substuff2", "123", Method_Not_Allowed);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");
}

TEST_F(RestApiPostTests, post_to_full_array_fails)
{
   auto bools = arrayTypes.Get_bool_types();
   for (size_t i = 0; i < bools.capacity() - 1; i++)
   {
      ASSERT_TRUE(bools.Add(true));
   }

   auto subs = arrayTypes.Get_submsg_types();
   for (size_t i = 0; i < subs.capacity() - 1; i++)
   {
      ASSERT_TRUE(subs.Add().has_value());
   }

   auto expected_index = bools.capacity() - 1; // zero-based index
   Verify_Array_Post(ADMIN, "/bool_type/", "true", Created, (jude_id_t)expected_index);
   Verify_Array_Post(ADMIN, "/bool_type/", "true", Bad_Request);
   
   auto expected_id = (jude_size_t)bools.capacity(); // 1-based "id"
   Verify_Array_Post(ADMIN, "/submsg_type/", "{}", Created, (jude_id_t)expected_id);
   Verify_Array_Post(ADMIN, "/submsg_type/", "{}", Bad_Request);
}

TEST_F(RestApiPostTests, post_to_array_adds_new_element)
{
   auto bools = arrayTypes.Get_bool_types();
   bools.Add(true);
   bools.Add(false);
   bools.Add(true);
   bools.Add(false);

   auto ints = arrayTypes.Get_int8_types();
   ints.Add(1);
   ints.Add(2);
   ints.Add(3);

   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3],"bool_type":[true,false,true,false]})");

   Verify_Array_Post(ADMIN, "/bool_type/", "true", Created, 4);  // we expect to be given the new index "4" as we've added the 5th item to the list
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3],"bool_type":[true,false,true,false,true]})");

   Verify_Array_Post(ADMIN, "/int8_type", "9", Created, 3);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3,9],"bool_type":[true,false,true,false,true]})");
}

TEST_F(RestApiPostTests, post_to_sub_object_array_creates_new_id)
{
   auto subObjects = arrayTypes.Get_submsg_types();
   subObjects.Add(10)->Set_substuff1("Hello,").Set_substuff2(3).Set_substuff3(true);
   subObjects.Add(20)->Set_substuff1("to_the").Set_substuff2(2).Set_substuff3(false);

   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false}
         ]
      }
   )");

   // Check our assumption on the customer uuid generator value before doing next tests...
   ASSERT_EQ(uuid_counter, 1);

   Verify_Array_Post(ADMIN, "/submsg_type/","{}", Created, 1);

   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":1}
         ]
      }
   )");

   Verify_Array_Post(ADMIN, "/submsg_type/", R"({"substuff1":"World!","substuff2":1,"substuff3":true})", Created, 2);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":1},
            {"id":2,"substuff1":"World!","substuff2":1,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Post(jude_user_Root, "/submsg_type/", R"({"id":25, "substuff2":123,"substuff3":false})", Created, 25); // should be able to dictate the "id" as Root
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":1},
            {"id":2,"substuff1":"World!","substuff2":1,"substuff3":true},
            {"id":25, "substuff2":123,"substuff3":false}
         ]
      }
   )");
}
