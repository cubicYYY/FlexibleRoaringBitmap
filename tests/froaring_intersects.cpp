#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

class FlexibleRoaringIntersectsTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FlexibleRoaringIntersectsTest, BothUninitialized) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    EXPECT_FALSE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, OtherUninitialized) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    EXPECT_FALSE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, ThisUninitialized) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    b.set(1);
    EXPECT_FALSE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, BothContainersIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);
    EXPECT_TRUE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, BothContainersNoIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(3);
    b.set(4);
    EXPECT_FALSE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, SingleContainerInThisIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    b.set(1);
    b.set(2);
    EXPECT_TRUE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, SingleContainerInThisNoIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    b.set(2);
    EXPECT_FALSE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, SingleContainerInOtherIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(1);
    EXPECT_TRUE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, SingleContainerInOtherNoIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(3);
    EXPECT_FALSE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, BothSingleContainersIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    b.set(1);
    EXPECT_TRUE(a.intersects(b));
}

TEST_F(FlexibleRoaringIntersectsTest, BothSingleContainersNoIntersect) {
    FlexibleRoaring<uint32_t, 16, 16> a;
    FlexibleRoaring<uint32_t, 16, 16> b;
    a.set(1);
    b.set(2);
    EXPECT_FALSE(a.intersects(b));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}