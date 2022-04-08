#include <gtest/gtest.h>

#include "jude/jude.h"

TEST(jude_bitfield, set_bit_0)
{
   // Given
   union {
      uint8_t mask[4];
      struct {
         bool bit_0 : 1;
         bool bit_1 : 1;
         bool bit_2 : 1;
         bool bit_3 : 1;
         bool bit_4 : 1;
         bool bit_5 : 1;
         bool bit_6 : 1;
         bool bit_7 : 1;
         bool bit_8 : 1;
         bool bit_9 : 1;
         bool bit_10 : 1;
         bool bit_11 : 1;
         bool bit_12 : 1;
         bool bit_13 : 1;
         bool bit_14 : 1;
         bool bit_15 : 1;
         bool bit_16 : 1;
         bool bit_17 : 1;
         bool bit_18 : 1;
         bool bit_19 : 1;
         bool bit_20 : 1;
         bool bit_21 : 1;
         bool bit_22 : 1;
         bool bit_23 : 1;
         bool bit_24 : 1;
         bool bit_25 : 1;
         bool bit_26 : 1;
         bool bit_27 : 1;
         bool bit_28 : 1;
         bool bit_29 : 1;
         bool bit_30 : 1;
         bool bit_31 : 1;
      } bits;
   } bitfield = { };

   memset(&bitfield, 0, sizeof(bitfield));

   EXPECT_FALSE(bitfield.bits.bit_0);

   // When
   jude_bitfield_set(bitfield.mask, 0);

   // Then
   EXPECT_TRUE(bitfield.bits.bit_0);
   EXPECT_FALSE(bitfield.bits.bit_1);
   EXPECT_EQ(0, bitfield.mask[1]);
   EXPECT_EQ(0, bitfield.mask[2]);
   EXPECT_FALSE(bitfield.bits.bit_25);

   // When
   jude_bitfield_set(bitfield.mask, 25);

   // Then
   EXPECT_TRUE(bitfield.bits.bit_25);
   EXPECT_FALSE(bitfield.bits.bit_26);
   EXPECT_EQ(0, bitfield.mask[1]);
   EXPECT_EQ(0, bitfield.mask[2]);
}
