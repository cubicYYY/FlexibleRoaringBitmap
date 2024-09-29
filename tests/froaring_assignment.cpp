#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

class FlexibleRoaringTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FlexibleRoaringTest, MoveAssignmentOperator) {
    FlexibleRoaring<uint32_t, 16, 16> bitmap1;
    bitmap1.set(1);
    bitmap1.set(2);

    FlexibleRoaring<uint32_t, 16, 16> bitmap2;
    bitmap2 = std::move(bitmap1);

    EXPECT_TRUE(bitmap2.test(1));
    EXPECT_TRUE(bitmap2.test(2));
}

TEST_F(FlexibleRoaringTest, CopyAssignmentOperator) {
    FlexibleRoaring<uint32_t, 16, 16> bitmap1;
    bitmap1.set(1);
    bitmap1.set(2);

    FlexibleRoaring<uint32_t, 16, 16> bitmap2;
    bitmap2 = bitmap1;

    EXPECT_TRUE(bitmap1.test(1));
    EXPECT_TRUE(bitmap1.test(2));
    EXPECT_TRUE(bitmap2.test(1));
    EXPECT_TRUE(bitmap2.test(2));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}