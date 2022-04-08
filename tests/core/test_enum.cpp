#include <gtest/gtest.h>

#include "jude/jude.h"
#include "autogen/alltypes_test.model.h"

const char     *jude_enum_find_string(const jude_enum_map_t *map, jude_enum_value_t value);
bool            jude_enum_contains_value(const jude_enum_map_t *map, jude_enum_value_t value);

/* FYI: from "../autogen/alltypes_test.jude.h"
TestEnum_Zero = 0,
TestEnum_First = 1,
TestEnum_Second = 2,
TestEnum_Truth = 42,
TestEnum__INVALID_VALUE
*/

TEST(jude_enum, count)
{
   ASSERT_EQ(4, jude_enum_count(TestEnum_enum_map));
}

TEST(jude_enum, get_value)
{
   ASSERT_EQ(jude::TestEnum::Zero,   jude_enum_get_value(TestEnum_enum_map, "Zero"));
   ASSERT_EQ(jude::TestEnum::First,  jude_enum_get_value(TestEnum_enum_map, "First"));
   ASSERT_EQ(jude::TestEnum::Second, jude_enum_get_value(TestEnum_enum_map, "Second"));
   ASSERT_EQ(jude::TestEnum::Truth,  jude_enum_get_value(TestEnum_enum_map, "Truth"));
}

TEST(jude_enum, find_value)
{
   ASSERT_EQ(&TestEnum_enum_map[0].value, jude_enum_find_value(TestEnum_enum_map, "Zero"));
   ASSERT_EQ(&TestEnum_enum_map[1].value, jude_enum_find_value(TestEnum_enum_map, "First"));
   ASSERT_EQ(&TestEnum_enum_map[2].value, jude_enum_find_value(TestEnum_enum_map, "Second"));
   ASSERT_EQ(&TestEnum_enum_map[3].value, jude_enum_find_value(TestEnum_enum_map, "Truth"));
   ASSERT_EQ(NULL, jude_enum_find_value(TestEnum_enum_map, "unknown"));
}

TEST(jude_enum, find_string)
{
   ASSERT_STREQ("Zero",   jude_enum_find_string(TestEnum_enum_map, jude::TestEnum::Zero  ));
   ASSERT_STREQ("First",  jude_enum_find_string(TestEnum_enum_map, jude::TestEnum::First ));
   ASSERT_STREQ("Second", jude_enum_find_string(TestEnum_enum_map, jude::TestEnum::Second));
   ASSERT_STREQ("Truth",  jude_enum_find_string(TestEnum_enum_map, jude::TestEnum::Truth ));
   ASSERT_EQ(NULL,     jude_enum_find_string(TestEnum_enum_map, 12345678       ));
}

TEST(jude_enum, contains_value)
{
   ASSERT_TRUE( jude_enum_contains_value(TestEnum_enum_map, jude::TestEnum::Zero  ));
   ASSERT_TRUE( jude_enum_contains_value(TestEnum_enum_map, jude::TestEnum::First ));
   ASSERT_TRUE( jude_enum_contains_value(TestEnum_enum_map, jude::TestEnum::Second));
   ASSERT_TRUE( jude_enum_contains_value(TestEnum_enum_map, jude::TestEnum::Truth ));
   ASSERT_FALSE(jude_enum_contains_value(TestEnum_enum_map, 12345678       ));
}
