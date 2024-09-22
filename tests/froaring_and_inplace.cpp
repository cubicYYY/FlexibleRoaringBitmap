#include <gtest/gtest.h>

#include "froaring.h"

namespace froaring {
TEST(FlexibleRoaringTest, AndInplace) {
    FlexibleRoaring<uint64_t, 16, 8> bitmap1;
    FlexibleRoaring<uint64_t, 16, 8> bitmap2;

    bitmap1.set(1);
    bitmap1.set(2);
    bitmap1.set(3);

    bitmap2.set(2);
    bitmap2.set(3);
    bitmap2.set(4);

    bitmap1 &= bitmap2;

    EXPECT_FALSE(bitmap1.test(1));
    EXPECT_TRUE(bitmap1.test(2));
    EXPECT_TRUE(bitmap1.test(3));
    EXPECT_FALSE(bitmap1.test(4));
}
}  // namespace froaring

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}