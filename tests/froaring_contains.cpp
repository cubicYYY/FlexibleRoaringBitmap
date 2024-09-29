#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

class FlexibleRoaringContainsTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FlexibleRoaringContainsTest, BothUninitialized) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    EXPECT_TRUE(a.contains(b));
}

TEST_F(FlexibleRoaringContainsTest, OtherUninitialized) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    EXPECT_TRUE(a.contains(b));
}

TEST_F(FlexibleRoaringContainsTest, ThisUninitialized) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    b.set(1);
    EXPECT_FALSE(a.contains(b));
}

TEST_F(FlexibleRoaringContainsTest, BothContainers) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(1);
    EXPECT_TRUE(a.contains(b));
    b.set(3);
    EXPECT_FALSE(a.contains(b));
}

TEST_F(FlexibleRoaringContainsTest, SingleContainerInThis) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    b.set(1);
    b.set(2);
    EXPECT_FALSE(a.contains(b));
}

TEST_F(FlexibleRoaringContainsTest, SingleContainerInOther) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(1);
    EXPECT_TRUE(a.contains(b));
    b.set(3);
    EXPECT_FALSE(a.contains(b));
}

TEST_F(FlexibleRoaringContainsTest, BothSingleContainers) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    b.set(1);
    EXPECT_TRUE(a.contains(b));
    b.set(2);
    EXPECT_FALSE(a.contains(b));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}