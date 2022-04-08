#include <gtest/gtest.h>

#include "jude/jude.h"
#include "test_base.h"

TEST(jude_common, jude_remove_const)
{
   // Given
   jude_object_t temp;
   temp.__child_index = 123;
   const jude_object_t *const_ptr = &temp;

   // When
   jude_object_t *ptr = jude_remove_const(const_ptr);

   // Then
   ASSERT_TRUE(ptr != NULL);
   ASSERT_EQ(ptr->__child_index, 123);
}
