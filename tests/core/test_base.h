#pragma once

#include <gtest/gtest.h>
#include <stdlib.h>

#include "jude/jude.h"
#include "jude/database/Database.h"
#include "jude/database/Resource.h"
#include "jude/database/Collection.h"

#include "autogen/alltypes_test.model.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "autogen/alltypes_test/SubMessage.h"
#include "autogen/alltypes_test/TagsTestSubArrays.h"

#include "autogen/alltypes_test/TestDB.h"

#define ASSERT_REST_OK(restOperation) \
   do { \
      auto res = restOperation; ASSERT_TRUE(res.IsOK()) << "Rest call '" << #restOperation << "' failed: " << res.GetDetails(); \
   } while(0)

#define ASSERT_REST_FAIL(restOperation) \
   do { \
      auto res = restOperation; ASSERT_FALSE(res.IsOK()) << "Rest call '" << #restOperation << "' was OK but expected to fail"; \
   } while(0)

#define ASSERT_REST_FAILS_WITH(restOperation,expectedError) \
   do { \
      auto res = restOperation; ASSERT_EQ(res.GetDetails(),expectedError) << "Rest call '" << #restOperation << "' was OK but expected to fail with: " << expectedError; \
   } while(0)

class TestDatabase : public jude::Database
{
public:

   TestDatabase(const std::string& name = "", 
      jude_user_t access = jude_user_Public, 
      std::shared_ptr<jude::Mutex> sharedMutex = std::make_shared<jude::Mutex>())
      : jude::Database(name, access, sharedMutex)
   {}
   bool AddToDB(jude::DatabaseEntry& entry)
   {
      return InstallDatabaseEntry(entry);
   }
};

class PopulatedDB : public jude::TestDB
{
public:
   PopulatedDB()
      : jude::TestDB("")
   {
      resource1->Set_substuff1("Hello");
      resource2->Set_substuff2(1234);

      collection1.Post(4);
      collection2.Post(5);
      
      subDB.resource->Set_substuff1("Hello");
      subDB.collection.Post(6);
   }
};

class JudeTestBase : public ::testing::Test
{

public:
   jude_id_t uuid_counter;

   jude::AllOptionalTypes optionals;
   jude::AllRepeatedTypes repeats;
   jude::AllOptionalTypes empty;
   jude::TagsTestSubArrays tagtests;

   AllOptionalTypes_t& ptrOptionals;
   AllRepeatedTypes_t& ptrRepeats;

   jude_object_t *optionals_object;
   jude_object_t *repeats_object;
   jude_object_t *empty_object;
   jude_object_t *tagtests_object;
   
   const jude_field_t *OptionalSubMessage;
   const jude_field_t *RepeatedSubMessage;

   JudeTestBase()
      : optionals(jude::AllOptionalTypes::New())
      , repeats(jude::AllRepeatedTypes::New())
      , empty(jude::AllOptionalTypes::New())
      , tagtests(jude::TagsTestSubArrays::New())
      , ptrOptionals(*(AllOptionalTypes_t*)optionals.RawData())
      , ptrRepeats(*(AllRepeatedTypes_t*)repeats.RawData())
   {
      jude_init();

      uuid_counter = 1;
      jude_install_custom_uuid_geneartor(this, [](void *data)->jude_id_t {
         JudeTestBase* _this = (JudeTestBase*)data;
         return _this->uuid_counter++;
      });

      tagtests.Clear();
      empty.Clear();

      optionals_object = optionals.RawData();
      repeats_object = repeats.RawData();
      empty_object = empty.RawData();
      tagtests_object = tagtests.RawData();

      OptionalSubMessage = jude_rtti_find_field(&AllOptionalTypes_rtti, "submsg_type");
      RepeatedSubMessage = jude_rtti_find_field(&AllRepeatedTypes_rtti, "submsg_type");
   }

   ~JudeTestBase()
   {
      jude_shutdown();
   }

   std::string StripSpaces(std::string& json)
   {
      json.erase(remove_if(json.begin(), json.end(), ::isspace), json.end());
      return json;
   }

   std::string StripSpaces(const char* json)
   {
      std::string myJson = json;
      return StripSpaces(myJson);
   }

   #define ASSERT_REPEATED_ADD(object, field, type)            \
      do {                                                     \
         auto fieldArray = object.Get_ ## field ## _types();   \
         auto oldCount = fieldArray.count();                   \
         auto res = fieldArray.Add(static_cast<type>(rand())); \
         if(!res) throw "Could not add " # field;              \
         auto newCount = fieldArray.count();                   \
         if (oldCount + 1 != newCount) throw "Unexpected count for " # field; \
      } while (0)

   void Initialise_AllRepeatedTypes(jude::AllRepeatedTypes &object)
   {

      ASSERT_REPEATED_ADD(object, int8,   int8_t);
      ASSERT_REPEATED_ADD(object, int16,  int16_t);
      ASSERT_REPEATED_ADD(object, int32,  int32_t);
      ASSERT_REPEATED_ADD(object, int64,  int64_t);

      ASSERT_REPEATED_ADD(object, uint8,  uint8_t );
      ASSERT_REPEATED_ADD(object, uint16, uint16_t);
      ASSERT_REPEATED_ADD(object, uint32, uint32_t);
      ASSERT_REPEATED_ADD(object, uint64, uint64_t);
      
      object.Get_bool_types().Add(true);
      
      object.Get_string_types().Add("Hello!");
      object.Get_string_types().count();

      object.Get_bytes_types().Add((const uint8_t*)"Hello, data!", 12);
      object.Get_bytes_types().count();

      auto submsg = object.Get_submsg_types().Add(123);
      submsg->Set_substuff1("Subway");

      ASSERT_REPEATED_ADD(object, enum, jude::TestEnum::Value);

      //BitMask8_t mask = { 0 };
      //auto submsg = object.Get_bitmask_types().Add(mask);
   }

   void Initialise_AllOptionalTypes(jude::AllOptionalTypes &object)
   {
      #define ASSERT_OPTIONAL_SET(object, field, type)                      \
         object.Set_ ## field ## _type((type)rand());                       \
         if (!object.Has_ ## field ## _type()) throw "Could not set " # field \

      ASSERT_OPTIONAL_SET(object, int8,   int8_t  );
      ASSERT_OPTIONAL_SET(object, int16,  int16_t );
      ASSERT_OPTIONAL_SET(object, int32,  int32_t);
      ASSERT_OPTIONAL_SET(object, int64,  int64_t);
      ASSERT_OPTIONAL_SET(object, uint8,  uint8_t );
      ASSERT_OPTIONAL_SET(object, uint16, uint16_t);
      ASSERT_OPTIONAL_SET(object, uint32, uint32_t);
      ASSERT_OPTIONAL_SET(object, uint64, uint64_t);
      
      object.Set_bool_type(true);
      if (!object.Has_bool_type()) throw "Could not set Bool";

      object.Set_string_type("Hello!");
      object.Has_string_type();

      object.Set_bytes_type((const uint8_t*)"Hello, data!", 12);
      object.Has_bytes_type();

      object.Get_submsg_type().Set_substuff1("Subway");

      ASSERT_OPTIONAL_SET(object, enum, jude::TestEnum::Value);
      object.Set_enum_type(jude::TestEnum::Second);
   }

   jude_object_t* object(jude::Object& object)
   {
      return object.RawData();
   }

   jude_object_t* object(void* object)
   {
      return (jude_object_t*)object;
   }
};

