#include <gtest/gtest.h>
#include <jude/restapi/jude_rest_api.h>
#include <algorithm>
#include <ctype.h>
#include <sstream>

#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

using namespace std;

class RestApiDeleteTests : public ::testing::Test
{
public:
   jude::AllOptionalTypes singleTypes = jude::AllOptionalTypes::New();
   jude::AllRepeatedTypes arrayTypes = jude::AllRepeatedTypes::New();

   std::stringstream mockInput;
   std::stringstream mockOutput;

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

   void Verify_Delete(jude_user_t user, const char* path, jude_restapi_code_t expected_code)
   {
      auto result_code = singleTypes.RestDelete(path, user).GetCode();
      ASSERT_EQ(result_code, expected_code) << "DELETE " << path << " resulted in unexpected code: " << result_code << endl;
   }

   void Verify_Array_Delete(jude_user_t user, const char* path, jude_restapi_code_t expected_code)
   {
      auto result_code = arrayTypes.RestDelete(path, user).GetCode();
      ASSERT_EQ(result_code, expected_code) << "DELETE " << path << " resulted in unexpected code: " << result_code << endl;
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

#define CHECK_GET_SINGLE_FIELD_OK(user, field, value, expected_Value) \
   AllOptionalTypes_api->set_ ## field(&optionals, value);            \
   Verify_Get(user, "/" #field, OK, #expected_Value)                  \

#define CHECK_GET_ARRAY_FIELD_OK(user, field, type, expected, ...) do {           \
   type values[] = { __VA_ARGS__ };                                  \
   size_t elementCount = sizeof(values) / sizeof(values[0]);         \
   AllRepeatedTypes_api->clear_ ## field ## s(&repeats);             \
   for (unsigned index = 0; index < elementCount; index++)           \
   {                                                                 \
      AllRepeatedTypes_api->add_ ## field(&repeats, values[index]);  \
   }                                                                 \
   Verify_Array_Get(user, "/" #field, OK, expected);                \
   } while(false)


TEST_F(RestApiDeleteTests, delete_bad_path_returns_404)
{
   Verify_Delete(ADMIN, "/does_not_exist", Not_Found);
   Verify_Array_Delete(ADMIN, "/does_not_exist", Not_Found);
}

TEST_F(RestApiDeleteTests, delete_root_not_allowed)
{
   singleTypes
      .Set_bool_type(true)
      .Set_int16_type(123)
      .Set_string_type("Hello");

   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Delete(ADMIN, "/", Forbidden);
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Delete(ADMIN, "", Forbidden);
   Verify_Get(ADMIN, "", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
}

TEST_F(RestApiDeleteTests, delete_individual_field)
{
   singleTypes
      .Set_bool_type(true)
      .Set_int16_type(123)
      .Set_string_type("Hello");

   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"bool_type":true,"string_type":"Hello"})");
   Verify_Delete(ADMIN, "/bool_type", OK);
   Verify_Get(ADMIN, "/", OK, R"({"int16_type":123,"string_type":"Hello"})");
   Verify_Delete(ADMIN, "/string_type", OK);
   Verify_Get(ADMIN, "", OK, R"({"int16_type":123})");
   Verify_Delete(ADMIN, "/int16_type", OK);
   Verify_Get(ADMIN, "", OK, R"({})");
}

TEST_F(RestApiDeleteTests, delete_sub_object_individual_fields)
{
   auto sub = singleTypes.Get_submsg_type();
   sub.Set_substuff1("Hello").Set_substuff2(32).Set_substuff3(true);

   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff2":32,"substuff3":true}})");
   Verify_Delete(ADMIN, "/submsg_type/substuff2", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff1":"Hello","substuff3":true}})");
   Verify_Delete(ADMIN, "/submsg_type/substuff1", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{"substuff3":true}})");
   Verify_Delete(ADMIN, "/submsg_type/substuff3", OK);
   Verify_Get(ADMIN, "/", OK, R"({"submsg_type":{}})");
   Verify_Delete(ADMIN, "/submsg_type/", OK);
   Verify_Get(ADMIN, "", OK, R"({})");
}

TEST_F(RestApiDeleteTests, delete_individual_element_of_array)
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
   Verify_Array_Delete(ADMIN, "/bool_type/1", OK); // deleete element "1" (zero based so this is 2nd element)
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[1,2,3,4],"bool_type":[true,true,false]})");
   Verify_Array_Delete(ADMIN, "/int8_type/0", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"int8_type":[2,3,4],"bool_type":[true,true,false]})");
   Verify_Array_Delete(ADMIN, "/int8_type", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"({"bool_type":[true,true,false]})");
   Verify_Array_Delete(ADMIN, "/int8_type/1", Not_Found);
   Verify_Array_Delete(ADMIN, "/bool_type/10", Not_Found);
}

TEST_F(RestApiDeleteTests, delete_individual_element_of_sub_object_array)
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

   Verify_Array_Delete(ADMIN, "/submsg_type/1", Not_Found);

   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":20,"substuff1":"to_the","substuff2":2,"substuff3":false},
            {"id":30,"substuff1":"World!","substuff2":1,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Delete(ADMIN, "/submsg_type/20", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true},
            {"id":30,"substuff1":"World!","substuff2":1,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Delete(ADMIN, "/submsg_type/30", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Delete(ADMIN, "/submsg_type/30", Not_Found);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[
            {"id":10,"substuff1":"Hello,","substuff2":3,"substuff3":true}
         ]
      }
   )");

   Verify_Array_Delete(ADMIN, "/submsg_type/10", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
         "submsg_type":[]
      }
   )");

   Verify_Array_Delete(ADMIN, "/submsg_type/", OK);
   Verify_Array_Get(ADMIN, "/", OK, R"(
      {  
      }
   )");
}
