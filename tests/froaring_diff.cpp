#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

class FlexibleRoaringBitmapDiffTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FlexibleRoaringBitmapDiffTest, DiffBothUninitialized) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    auto result = a - b;
    ASSERT_FALSE(result.is_inited());
}

TEST_F(FlexibleRoaringBitmapDiffTest, DiffLeftUninitialized) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    b.set(1);
    auto result = a - b;
    ASSERT_FALSE(result.is_inited());
}

TEST_F(FlexibleRoaringBitmapDiffTest, DiffRightUninitialized) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    a.set(1);
    auto result = a - b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
}

TEST_F(FlexibleRoaringBitmapDiffTest, DiffBothSingleContainer) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);
    auto result = a - b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
    ASSERT_FALSE(result.test(2));
    ASSERT_FALSE(result.test(3));
}

TEST_F(FlexibleRoaringBitmapDiffTest, DiffBothContainers) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    a.set(1);
    a.set(10000);
    b.set(2);
    b.set(10000);
    auto result = a - b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
    ASSERT_FALSE(result.test(10000));
    ASSERT_FALSE(result.test(2));
}

TEST_F(FlexibleRoaringBitmapDiffTest, DiffOneContainer) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    a.set(1);
    a.set(10000);
    b.set(2);
    auto result = a - b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
    ASSERT_TRUE(result.test(10000));
    ASSERT_FALSE(result.test(2));
}

TEST_F(FlexibleRoaringBitmapDiffTest, DiffRangeTest) {
    FlexibleRoaringBitmap<uint32_t, 16, 16> a;
    FlexibleRoaringBitmap<uint32_t, 16, 16> b;
    for (uint32_t i = 200; i <= 260; ++i) {
        a.set(i);
    }
    for (uint32_t i = 263; i <= 513; ++i) {
        b.set(i);
    }
    auto result = a - b;
    ASSERT_TRUE(result.is_inited());
    for (uint32_t i = 200; i <= 260; ++i) {
        ASSERT_TRUE(result.test(i));
    }
    for (uint32_t i = 263; i <= 513; ++i) {
        ASSERT_FALSE(result.test(i));
    }
    ASSERT_FALSE(result.test(261));
    ASSERT_FALSE(result.test(262));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}