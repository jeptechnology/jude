#include <gtest/gtest.h>
#include <jude/restapi/jude_rest_api.h>
#include <algorithm>
#include <ctype.h>

#include "streams/mock_istream.h"
#include "streams/mock_ostream.h"

#include "core/test_base.h"
#include "autogen/alltypes_test.model.h"

using namespace std;

class RestApiGetTests : public JudeTestBase
{
public:
    MockInputStream mockInput;
    MockOutputStream mockOutput;

    jude_user_t ADMIN  = jude_user_Admin;
    jude_user_t PUBLIC = jude_user_Public;

    jude_restapi_code_t OK                    = jude_rest_OK;
    jude_restapi_code_t Created               = jude_rest_Created;
    jude_restapi_code_t No_Content            = jude_rest_No_Content;
    jude_restapi_code_t Bad_Request           = jude_rest_Bad_Request;
    jude_restapi_code_t Unauthorized          = jude_rest_Unauthorized;
    jude_restapi_code_t Forbidden             = jude_rest_Forbidden;
    jude_restapi_code_t Not_Found             = jude_rest_Not_Found;
    jude_restapi_code_t Method_Not_Allowed    = jude_rest_Method_Not_Allowed;
    jude_restapi_code_t Conflict              = jude_rest_Conflict;
    jude_restapi_code_t Internal_Server_Error = jude_rest_Internal_Server_Error;

    std::string lastOutput; 

    string& StripSpaces(string& json)
    {
        json.erase(remove_if(json.begin(), json.end(), ::isspace), json.end());
        return json;
    }

    void Verify_Get(jude_user_t user, 
                         const char *path, 
                         jude_restapi_code_t expected_code, 
                         const char *expected_result = nullptr)
    {
        MockOutputStream mos(256);
        auto result_code = jude_restapi_get(user, this->optionals_object, path, mos.GetLowLevelOutputStream());
        mos.Flush();
        ASSERT_EQ(result_code, expected_code) << "GET " << path << " resulted in unexpected code: " << result_code << endl << "Stream error: " << mos.GetOutputErrorMsg();
        if (expected_result)
        {
            EXPECT_STREQ(expected_result, mos.GetOutputString().c_str()) << "GET " << path << " resulted in unexpected response";
        }

        lastOutput = mos.GetOutputString();
    }

    void Verify_Array_Get(jude_user_t user, 
                         const char *path, 
                         jude_restapi_code_t expected_code, 
                         const char *expected_result = nullptr)
    {
        MockOutputStream mos(256);
        auto result_code = jude_restapi_get(user, this->repeats_object, path, mos.GetLowLevelOutputStream());
        mos.Flush();

        ASSERT_EQ(result_code, expected_code) << "GET " << path << " resulted in unexpected code: " << result_code;
        if (expected_result)
        {
            string expected = expected_result;
            StripSpaces(expected);
            EXPECT_STREQ(expected.c_str(), mos.GetOutputString().c_str()) << "GET " << path << " resulted in unexpected response";
        }

        lastOutput = mos.GetOutputString();
    }

};

#define CHECK_GET_SINGLE_FIELD_OK(user, field, value, expected_Value) \
   optionals.Set_ ## field(value);                                    \
   Verify_Get(user, "/" #field, OK, #expected_Value)                  \

#define CHECK_GET_ARRAY_FIELD_OK(user, field, type, expected, ...) do {           \
   type values[] = { __VA_ARGS__ };                                  \
   size_t elementCount = sizeof(values) / sizeof(values[0]);         \
   auto myArray = repeats.Get_ ## field ## s();                      \
   myArray.clear();                                                  \
   for (unsigned index = 0; index < elementCount; index++)           \
   {                                                                 \
      myArray.Add(values[index]);                                    \
   }                                                                 \
   Verify_Array_Get(user, "/" #field, OK, expected);                 \
   } while(false)


TEST_F(RestApiGetTests, get_bad_path_returns_404)
{
   Verify_Get(ADMIN, "/does_not_exist", Not_Found);
}

TEST_F(RestApiGetTests, get_slash_gets_everything)
{
   Verify_Get(ADMIN, "/", OK);
}

TEST_F(RestApiGetTests, get_empty_path_gets_everything)
{
   Verify_Get(ADMIN, "", OK);
}

TEST_F(RestApiGetTests, get_null_path_gets_everything)
{
   Verify_Get(ADMIN, nullptr, OK);
}

TEST_F(RestApiGetTests, int8_type)
{
    CHECK_GET_SINGLE_FIELD_OK(ADMIN, int8_type,  0, 0);
    CHECK_GET_SINGLE_FIELD_OK(ADMIN, int8_type,  5, 5);
    CHECK_GET_SINGLE_FIELD_OK(ADMIN, int8_type, -5,-5);
    CHECK_GET_SINGLE_FIELD_OK(ADMIN, int8_type, INT8_MIN, -128);
    CHECK_GET_SINGLE_FIELD_OK(ADMIN, int8_type, INT8_MAX, 127);
}


TEST_F(RestApiGetTests, int16_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int16_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int16_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int16_type, -5, -5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int16_type, INT16_MIN, -32768);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int16_type, INT16_MAX, 32767);
}

TEST_F(RestApiGetTests, int32_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int32_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int32_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int32_type, -5, -5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int32_type, INT32_MIN, -2147483648);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int32_type, INT32_MAX, 2147483647);
}

TEST_F(RestApiGetTests, int64_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int64_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int64_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int64_type, -5, -5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int64_type, INT64_MIN, -9223372036854775808);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, int64_type, INT64_MAX, 9223372036854775807);
}

TEST_F(RestApiGetTests, uint8_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint8_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint8_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint8_type, UINT8_MAX, 255);
}

TEST_F(RestApiGetTests, uint16_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint16_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint16_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint16_type, UINT16_MAX, 65535);
}

TEST_F(RestApiGetTests, uint32_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint32_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint32_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint32_type, UINT32_MAX, 4294967295);
}

TEST_F(RestApiGetTests, uint64_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint64_type, 0, 0);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint64_type, 5, 5);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, uint64_type, UINT64_MAX, 18446744073709551615);
}

TEST_F(RestApiGetTests, bool_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, bool_type, 0, false);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, bool_type, 1, true);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, bool_type, false, false);
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, bool_type, true, true);
}

TEST_F(RestApiGetTests, string_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, string_type, "", "");
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, string_type, "Hello", "Hello");
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, string_type, "\"", "\"");
}

TEST_F(RestApiGetTests, enum_type)
{
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, enum_type, jude::TestEnum::First, "First");
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, enum_type, jude::TestEnum::Second, "Second");
   CHECK_GET_SINGLE_FIELD_OK(ADMIN, enum_type, jude::TestEnum::Zero, "Zero");
}

TEST_F(RestApiGetTests, submessage_type)
{
   optionals.Get_submsg_type().Set_substuff1("Hello");
   optionals.Get_submsg_type().Set_substuff2(12345678);
   optionals.Get_submsg_type().Set_substuff3(true);
   Verify_Get(ADMIN, "/submsg_type", OK, R"({"substuff1":"Hello","substuff2":12345678,"substuff3":true})");
   Verify_Get(ADMIN, "/submsg_type/substuff1", OK, "\"Hello\"");
   Verify_Get(ADMIN, "/submsg_type/substuff2", OK, "12345678");
   Verify_Get(ADMIN, "/submsg_type/substuff3", OK, "true");
   Verify_Get(ADMIN, "/submsg_type/unknown", Not_Found);
}


TEST_F(RestApiGetTests, int8_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int8_type, int8_t, "[0]",        0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int8_type, int8_t, "[0,1]",      0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int8_type, int8_t, "[5,-5]",     5, -5);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int8_type, int8_t, "[-128,127]", INT8_MIN, INT8_MAX);
}

TEST_F(RestApiGetTests, int16_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int16_type, int16_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int16_type, int16_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int16_type, int16_t, "[5,-5]", 5, -5);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int16_type, int16_t, "[-32768,32767]", INT16_MIN, INT16_MAX);
}

TEST_F(RestApiGetTests, int32_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int32_type, int32_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int32_type, int32_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int32_type, int32_t, "[5,-5]", 5, -5);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int32_type, int32_t, "[-2147483648,2147483647]", INT32_MIN, INT32_MAX);
}

TEST_F(RestApiGetTests, int64_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int64_type, int64_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int64_type, int64_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int64_type, int64_t, "[5,-5]", 5, -5);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, int64_type, int64_t, "[-9223372036854775808,9223372036854775807]", INT64_MIN, INT64_MAX);
}

TEST_F(RestApiGetTests, uint8_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint8_type, uint8_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint8_type, uint8_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint8_type, uint8_t, "[0,1,2,255]", 0, 1, 2, UINT8_MAX);
}

TEST_F(RestApiGetTests, uint16_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint16_type, uint16_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint16_type, uint16_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint16_type, uint16_t, "[0,1,2,65535]", 0, 1, 2, UINT16_MAX);
}

TEST_F(RestApiGetTests, uint32_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint32_type, uint32_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint32_type, uint32_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint32_type, uint32_t, "[0,1,2,4294967295]", 0, 1, 2, UINT32_MAX);
}

TEST_F(RestApiGetTests, uint64_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint64_type, uint64_t, "[0]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint64_type, uint64_t, "[0,1]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, uint64_type, uint64_t, "[0,1,2,18446744073709551615]", 0, 1, 2, UINT64_MAX);
}

TEST_F(RestApiGetTests, bool_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, bool_type, bool, "[false]", 0);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, bool_type, bool, "[false,true]", 0, 1);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, bool_type, bool, "[true,false,true]", true, false, true);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, bool_type, bool, "[true,true,false,false]", 1, true, 0, false);
}

TEST_F(RestApiGetTests, string_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, string_type, const char*, R"([""])", "");
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, string_type, const char*, R"(["","Hello"])", "", "Hello");
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, string_type, const char*, R"(["Hello","World"])", "Hello", "World");
}

TEST_F(RestApiGetTests, enum_type_array)
{
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, enum_type, jude::TestEnum::Value, R"(["First"])", jude::TestEnum::First);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, enum_type, jude::TestEnum::Value, R"(["First","Second","Zero"])", jude::TestEnum::First, jude::TestEnum::Second, jude::TestEnum::Zero);
   CHECK_GET_ARRAY_FIELD_OK(ADMIN, enum_type, jude::TestEnum::Value, R"(["First"])", jude::TestEnum::First);
}

TEST_F(RestApiGetTests, submessage_type_array)
{
   auto subArray = repeats.Get_submsg_types();
   subArray.clear();

   subArray.Add(1)->Set_substuff1("Hello1")
                   .Set_substuff2(1001)
                   .Set_substuff3(false);

   subArray.Add(2)->Set_substuff1("Hello2")
                   .Set_substuff3(true);
   
   subArray.Add(3)->Set_substuff1("Hello3")
                   .Set_substuff2(1003);

   Verify_Array_Get(ADMIN, "/submsg_type", OK, R"(
        [
            {"id":1,"substuff1":"Hello1","substuff2":1001,"substuff3":false },
            {"id":2,"substuff1":"Hello2",                 "substuff3":true  },
            {"id":3,"substuff1":"Hello3","substuff2":1003}
        ]
   )");

   Verify_Array_Get(ADMIN, "/submsg_type/1", OK, R"(
            {"id":1,"substuff1":"Hello1","substuff2":1001,"substuff3":false }
   )");
   Verify_Array_Get(ADMIN, "/submsg_type/2", OK, R"(
            {"id":2,"substuff1":"Hello2",                 "substuff3":true  }
   )");
   Verify_Array_Get(ADMIN, "/submsg_type/3", OK, R"(
            {"id":3,"substuff1":"Hello3","substuff2":1003}
   )");

   Verify_Array_Get(ADMIN, "/submsg_type/1/substuff1", OK, "\"Hello1\"");
   Verify_Array_Get(ADMIN, "/submsg_type/1/substuff2", OK, "1001");
   Verify_Array_Get(ADMIN, "/submsg_type/1/substuff3", OK, "false");
   Verify_Array_Get(ADMIN, "/submsg_type/1/unknown", Not_Found);
}
