#include <gtest/gtest.h>
#include <jude/jude.h>
#include <jude/restapi/jude_rest_api.h>
#include <algorithm>
#include <ctype.h>

#include "core/test_base.h"
#include "jude/core/cpp/Stream.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"

using namespace std;

class JsonSchemaTests : public JudeTestBase
{
public:
   std::stringstream ss;
   jude::OutputStreamWrapper mockOutput{ss};

   string StripSpaces(string& json)
   {
      json.erase(remove_if(json.begin(), json.end(), ::isspace), json.end());
      return json;
   }
   string StripSpaces(const char *json)
   {
      std::string myJson = json;
      return StripSpaces(myJson);
   }
};

TEST_F(JsonSchemaTests, Successful_json_schema_call_returns_true)
{
   auto result = jude_create_default_json_schema(&mockOutput.m_ostream, jude::SubMessage::RTTI(), jude_user_Public);
   ASSERT_TRUE(result);
}

TEST_F(JsonSchemaTests, Can_Create_SubMessage_schema)
{
   auto result = jude_create_default_json_schema(&mockOutput.m_ostream, jude::SubMessage::RTTI(), jude_user_Public);
   ASSERT_TRUE(result);
   ASSERT_TRUE(ss.str().length() > 0);

   auto expectedJsonSchema = StripSpaces(R"(
      {
         "type": "object",
         "allOf": [
            {
               "$ref": "#/$defs/SubMessage"
            }
         ],
         "$defs": {
            "SubMessage": {
               "type": "object",
               "properties": {
                  "id": {
                     "readOnly": true,
                     "type": "integer",
                     "minimum": 0
                  },
                  "substuff1": {
                     "type": "string",
                     "maxLength": 63
                  },
                  "substuff2": {
                     "type": "integer"
                  },
                  "substuff3": {
                     "type": "boolean"
                  }
               }
            }
         }
      }
   )");

   ASSERT_STREQ(ss.str().c_str(), expectedJsonSchema.c_str());
}

TEST_F(JsonSchemaTests, Can_Create_AllOptional_schema)
{
   auto result = jude_create_default_json_schema(&mockOutput.m_ostream, jude::AllOptionalTypes::RTTI(), jude_user_Public);
   ASSERT_TRUE(result);
   ASSERT_TRUE(ss.str().length() > 0);

   auto expectedJsonSchema = StripSpaces(R"(
      { 
         "type":"object",
         "allOf": [
            { "$ref":"#/$defs/AllOptionalTypes" } 
         ],
         "$defs": {
            "AllOptionalTypes": {
               "type": "object",
               "properties": {
                  "id":          { "readOnly":true, "type": "integer", "minimum": 0 },
                  "int8_type":   { "type": "integer", "minimum": -128,   "maximum": 127    },
                  "int16_type":  { "type": "integer", "minimum": -32768, "maximum": 32767    },
                  "int32_type":  { "type": "integer" },    
                  "int64_type":  { "type": "integer" },
                  "uint8_type":  { "type": "integer", "minimum": 0, "maximum": 255    },
                  "uint16_type": { "type": "integer", "minimum": 0, "maximum": 65535    },
                  "uint32_type": { "type": "integer", "minimum": 0  },
                  "uint64_type": { "type": "integer", "minimum": 0  },
                  "bool_type":   { "type": "boolean" },
                  "string_type": { "type": "string",   "maxLength": 31    },
                  "bytes_type":  { "type": "string",   "maxLength": 48    },
                  "submsg_type": { "$ref": "#/$defs/SubMessage" },    
                  "enum_type":   { 
                     "type": "string",   
                     "enum": [ "Zero", "First", "Second", "Truth"]
                  },
                  "bitmask_type":{      
                     "type": "object",      
                     "properties": {
                        "BitZero":  { "type": "boolean" },
                        "BitOne":   { "type": "boolean" },
                        "BitTwo":   { "type": "boolean" },
                        "BitThree": { "type": "boolean" },
                        "BitFour":  { "type": "boolean" },
                        "BitFive":  { "type": "boolean" },
                        "BitSix":   { "type": "boolean" },
                        "BitSeven": { "type": "boolean" }
                     }
                  }
               }
            },
            "SubMessage": {
               "type": "object",
               "properties": {
                  "id":        { "readOnly":true, "type": "integer", "minimum":0  },
                  "substuff1": { "type": "string",  "maxLength":63   },
                  "substuff2": { "type": "integer" },
                  "substuff3": { "type": "boolean" }
               }
            }
         }
      }
   )");

   ASSERT_STREQ(ss.str().c_str(), expectedJsonSchema.c_str());
}

TEST_F(JsonSchemaTests, Can_Create_AllRepeated_schema)
{
   auto result = jude_create_default_json_schema(&mockOutput.m_ostream, jude::AllRepeatedTypes::RTTI(), jude_user_Public);
   ASSERT_TRUE(result);
   ASSERT_TRUE(ss.str().length() > 0);

   auto expectedJsonSchema = StripSpaces(R"(
      { 
         "type": "object",
         "allOf": [
            {
               "$ref": "#/$defs/AllRepeatedTypes"
            }
         ],
         "$defs": {
            "AllRepeatedTypes": {
               "type": "object",
               "properties": {
                  "id":          { "readOnly":true, "type": "integer", "minimum": 0 },
                  "int8_type":   { "type": "array", "maxItems":32, "items": { "type": "integer", "minimum": -128,   "maximum": 127    }},
                  "int16_type":  { "type": "array", "maxItems":32, "items": { "type": "integer", "minimum": -32768, "maximum": 32767  }},
                  "int32_type":  { "type": "array", "maxItems":32, "items": { "type": "integer" }},    
                  "int64_type":  { "type": "array", "maxItems":32, "items": { "type": "integer" }},
                  "uint8_type":  { "type": "array", "maxItems":32, "items": { "type": "integer", "minimum": 0, "maximum": 255    }},
                  "uint16_type": { "type": "array", "maxItems":32, "items": { "type": "integer", "minimum": 0, "maximum": 65535  }},
                  "uint32_type": { "type": "array", "maxItems":32, "items": { "type": "integer", "minimum": 0  }},
                  "uint64_type": { "type": "array", "maxItems":32, "items": { "type": "integer", "minimum": 0  }},
                  "bool_type":   { "type": "array", "maxItems":32, "items": { "type": "boolean" }},
                  "string_type": { "type": "array", "maxItems":32, "items": { "type": "string",   "maxLength": 31 }},
                  "bytes_type":  { "type": "array", "maxItems":32, "items": { "type": "string",   "maxLength": 48 }},
                  "submsg_type": { "type": "array", "maxItems":32, "items": { "$ref": "#/$defs/SubMessage" }},    
                  "enum_type":   { "type": "array", "maxItems":32, "items": {
                     "type": "string",   
                     "enum": [ "Zero", "First", "Second", "Truth"]
                  }},
                  "bitmask_type": { "type": "array", "maxItems":32, "items": {      
                     "type": "object",      
                     "properties": {
                        "BitZero":  { "type": "boolean" },
                        "BitOne":   { "type": "boolean" },
                        "BitTwo":   { "type": "boolean" },
                        "BitThree": { "type": "boolean" },
                        "BitFour":  { "type": "boolean" },
                        "BitFive":  { "type": "boolean" },
                        "BitSix":   { "type": "boolean" },
                        "BitSeven": { "type": "boolean" }
                     }
                  }}
               }
            },
            "SubMessage": {
               "type": "object",
               "properties": {
                  "id":        { "readOnly":true, "type": "integer", "minimum":0  },
                  "substuff1": { "type": "string",  "maxLength":63   },
                  "substuff2": { "type": "integer" },
                  "substuff3": { "type": "boolean" }
               }
            }
         }
      }
   )");

   ASSERT_STREQ(ss.str().c_str(), expectedJsonSchema.c_str());
}
