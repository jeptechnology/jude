#include <gtest/gtest.h>
#include <jude/restapi/jude_rest_api.h>
#include <algorithm>
#include <ctype.h>
#include <sstream>

#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

using namespace std;

class RestApiPatchTests : public ::testing::Test
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

   void Verify_Patch(jude_user_t user, const char* fullpath, const char *inputJSON, jude_restapi_code_t expected_code)
   {
      stringstream mis(inputJSON);
      auto result = singleTypes.RestPatch(fullpath, mis, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "PATCH " << fullpath << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
   }

   void Verify_Array_Patch(jude_user_t user, const char* fullpath, const char* inputJSON, jude_restapi_code_t expected_code)
   {
      stringstream mis(inputJSON);
      auto result = arrayTypes.RestPatch(fullpath, mis, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "PATCH " << fullpath << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
   }

   void Verify_Get(jude_user_t user,
      const char* fullpath,
      jude_restapi_code_t expected_code,
      const char* expected_result = nullptr)
   {
      stringstream mos;
      auto result = singleTypes.RestGet(fullpath, mos, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "GET " << fullpath << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
      if (expected_result)
      {
         EXPECT_STREQ(expected_result, mos.str().c_str()) << "GET " << fullpath << " resulted in unexpected response";
      }

      lastOutput = mos.str();
   }

   void Verify_Array_Get(jude_user_t user,
      const char* fullpath,
      jude_restapi_code_t expected_code,
      const char* expected_result = nullptr)
   {
      stringstream mos;
      auto result = arrayTypes.RestGet(fullpath, mos, user);
      ASSERT_EQ(result.GetCode(), expected_code) << "GET " << fullpath << " resulted in unexpected code: " << result.GetCode() << endl << "Stream error: " << result.GetDetails();
      if (expected_result)
      {
         string expected = expected_result;
         StripSpaces(expected);
         EXPECT_STREQ(expected.c_str(), mos.str().c_str()) << "GET " << fullpath << " resulted in unexpected response";
      }

      lastOutput = mos.str();
   }
};

TEST_F(RestApiPatchTests, patch_bad_path_returns_404)
{
   Verify_Patch(ADMIN, "/does_not_exist", "{}", Not_Found);
   Verify_Array_Patch(ADMIN, "/does_not_exist", "{}", Not_Found);
}

TEST_F(RestApiPatchTests, patch_root_works)
{
   singleTypes
      .Set_bool_type(true)
      .Set_int16_type(123)
      .Set_string_type("Hello");

   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Patch(ADMIN, "/", R"({"bool_type":false,"string_type":"World"})", OK);
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":false,"string_type":"World"})");
   Verify_Patch(ADMIN, "/", R"({"int16_type":456})", OK);
   Verify_Get(ADMIN, "", OK, R"({"int16_type":456,"bool_type":false,"string_type":"World"})");
}

TEST_F(RestApiPatchTests, patch_individual_field)
{
   singleTypes
      .Set_bool_type(true)
      .Set_int16_type(123)
      .Set_string_type("Hello");

   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Patch(ADMIN, "/bool_type", "false", OK);
   Verify_Patch(ADMIN, "/string_type/", R"("World")", OK);
   Verify_Patch(ADMIN, "/string_type/", "World", OK); // NOTE: We can omit quotes for single patch
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":false,"string_type":"World"})");
   Verify_Patch(ADMIN, "/int16_type", "456", OK);
   Verify_Get(ADMIN, "", OK, R"({"int16_type":456,"bool_type":false,"string_type":"World"})");
   Verify_Patch(ADMIN, "/int8_type", "-1", OK);
   Verify_Get(ADMIN, "", OK, R"({"int8_type":-1,"int16_type":456,"bool_type":false,"string_type":"World"})");
}

TEST_F(RestApiPatchTests, patch_sub_object_individual_fields)
{
   auto sub = singleTypes.Get_submsg_type();
   sub.Set_substuff1("Hello").Set_substuff2(32).Set_substuff3(true);

   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");
   Verify_Patch(ADMIN, "/submsg_type/", R"({"substuff2":55})", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":55,"substuff3":true}})");
   Verify_Patch(ADMIN, "/submsg_type/", R"({"substuff3":false,"substuff1":"World"})", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"World","substuff2":55,"substuff3":false}})");
   Verify_Patch(ADMIN, "/submsg_type/substuff2", "123", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"World","substuff2":123,"substuff3":false}})");
}


TEST_F(RestApiPatchTests, patch_null_field_will_scrub_the_field)
{
   auto sub = singleTypes.Get_submsg_type();
   sub.Set_substuff1("Hello").Set_substuff2(32).Set_substuff3(true);

   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");

   // Patching "null" will remove the value
   Verify_Patch(ADMIN, "/submsg_type/substuff2", "null", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff3":true}})");
   ASSERT_FALSE(singleTypes.Get_submsg_type().Has_substuff2());

   // Patch one field to null, other field to new value...
   Verify_Patch(ADMIN, "/submsg_type/", R"({"substuff1":"World","substuff3":null})", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"World"}})");
   ASSERT_FALSE(singleTypes.Get_submsg_type().Has_substuff3());

   // Patch field to null directly
   Verify_Patch(ADMIN, "/submsg_type/substuff1", "null", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{}})");
   ASSERT_FALSE(singleTypes.Get_submsg_type().Has_substuff1());
}

TEST_F(RestApiPatchTests, patch_individual_element_of_array)
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
   ints.Add(4);

   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3,4],"bool_type":[true,false,true,false]})");
   
   Verify_Array_Patch(ADMIN, "/bool_type/1", "true", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3,4],"bool_type":[true,true,true,false]})");
   
   Verify_Array_Patch(ADMIN, "/int8_type/0", "9", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[9,2,3,4],"bool_type":[true,true,true,false]})");
   
   Verify_Array_Patch(ADMIN, "/int8_type/5", "1", Not_Found);
   Verify_Array_Patch(ADMIN, "/bool_type/10", "", Not_Found);
}

TEST_F(RestApiPatchTests, patch_whole_array_of_elements)
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
   ints.Add(4);

   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3,4],"bool_type":[true,false,true,false]})");

   Verify_Array_Patch(ADMIN, "/bool_type/", "[false,true,false]", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3,4],"bool_type":[false,true,false]})");

   Verify_Array_Patch(ADMIN, "/int8_type/", "[]", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[],"bool_type":[false,true,false]})");

   Verify_Array_Patch(ADMIN, "/string_type/", R"(["Hello","World!"])", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[],"bool_type":[false,true,false],"string_type":["Hello","World!"]})");
}

TEST_F(RestApiPatchTests, patch_individual_element_of_sub_object_array)
{
   auto subObjects = arrayTypes.Get_submsg_types();
   subObjects.Add(10)->Set_substuff1("Hello,").Set_substuff2(3).Set_substuff3(true);
   subObjects.Add(20)->Set_substuff1("to_the").Set_substuff2(2).Set_substuff3(false);
   subObjects.Add(30)->Set_substuff1("World!").Set_substuff2(1).Set_substuff3(true);

   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":30,"substuff1":"World!","substuff2":1,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Patch(ADMIN, "/submsg_type/1/substuff2","8", Not_Found);

   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":30,"substuff1":"World!","substuff2":1,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Patch(ADMIN, "/submsg_type/10/substuff2", "8", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":8,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":30,"substuff1":"World!","substuff2":1,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Patch(ADMIN, "/submsg_type/30", R"({"substuff2":123,"substuff3":false})", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":8,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":30,"substuff1":"World!","substuff2":123,"substuff3":false}
         ]
      }
   )");

   Verify_Array_Patch(ADMIN, "/submsg_type/30/substuff1", "Hello,world!", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":8,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":30,"substuff1":"Hello,world!","substuff2":123,"substuff3":false}
         ]
      }
   )");
}
