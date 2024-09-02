#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

TEST(FlexibleRoaringBitmapTest, RemoveAndCheckEmptyContainers) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> bitmap;

    bitmap.set(1);
    bitmap.set(2);
    bitmap.set(3);

    EXPECT_TRUE(bitmap.test(1));
    EXPECT_TRUE(bitmap.test(2));
    EXPECT_TRUE(bitmap.test(3));

    bitmap.reset(3);
    bitmap.set(114514);
    bitmap.reset(2);
    bitmap.set(1919810);
    bitmap.reset(1);

    EXPECT_FALSE(bitmap.test(1));
    EXPECT_FALSE(bitmap.test(2));
    EXPECT_FALSE(bitmap.test(3));
    EXPECT_FALSE(bitmap.test(1919809));
    EXPECT_FALSE(bitmap.test(1919811));
    EXPECT_TRUE(bitmap.test(1919810));

    EXPECT_EQ(bitmap.count(), 2);
}

TEST(FlexibleRoaringBitmapTest, TypeConversion) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> bitmap;

    for (auto i = 0; i < 1000; i++) {
        bitmap.set(i);
        bitmap.set(i + 4090);
    }
    bitmap.debug_print();
    EXPECT_EQ(bitmap.count(), 2000);

    for (auto i = 0; i < 1000; i++) {
        bitmap.set(i);
        bitmap.set(i + 4090);
    }

    EXPECT_EQ(bitmap.count(), 2000);

    for (auto i = 0; i < 1000; i++) {
        bitmap.reset(i);
        bitmap.reset(i + 4090);
    }

    EXPECT_EQ(bitmap.count(), 0);
}

TEST(FlexibleRoaringBitmapTest, TestAndSet) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> bitmap;

    for (auto i = 0; i < 1000; i++) {
        EXPECT_TRUE(bitmap.test_and_set(i));
        EXPECT_TRUE(bitmap.test_and_set(i + 4090));
    }

    for (auto i = 0; i < 1000; i++) {
        EXPECT_FALSE(bitmap.test_and_set(i));
        EXPECT_FALSE(bitmap.test_and_set(i + 4090));
    }

    EXPECT_EQ(bitmap.count(), 2000);
}

TEST(FlexibleRoaringBitmapTest, AndOperatorBothEmpty) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    auto result = bitmap1 & bitmap2;
    EXPECT_FALSE(result.is_inited());
}

TEST(FlexibleRoaringBitmapTest, AndOperatorOneEmpty) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    bitmap2.set(1);
    auto result = bitmap1 & bitmap2;
    EXPECT_FALSE(result.is_inited());
}

TEST(FlexibleRoaringBitmapTest, AndOperatorBothNonEmpty) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    bitmap1.set(1);
    bitmap1.set(2);
    bitmap2.set(2);
    bitmap2.set(3);
    auto result = bitmap1 & bitmap2;
    EXPECT_TRUE(result.is_inited());
    EXPECT_TRUE(result.test(2));
    EXPECT_FALSE(result.test(1));
    EXPECT_FALSE(result.test(3));
}

TEST(FlexibleRoaringBitmapTest, AndOperatorDifferentIndexes) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    bitmap1.set(1);
    bitmap2.set(1 << 16);  // Different index
    auto result = bitmap1 & bitmap2;
    EXPECT_FALSE(result.is_inited());
}

TEST(FlexibleRoaringBitmapTest, AndOperatorSameIndexes) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    bitmap1.set(114514);
    bitmap2.set(114514);
    auto result = bitmap1 & bitmap2;
    EXPECT_TRUE(result.is_inited());
    EXPECT_TRUE(result.test(114514));
}

TEST(FlexibleRoaringBitmapTest, AndOperatorContainers) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    bitmap1.set(114514);
    bitmap2.set(114514);
    bitmap1.set(999);
    bitmap2.set(999);
    auto result = bitmap1 & bitmap2;
    result.debug_print();
    EXPECT_TRUE(result.is_inited());
    EXPECT_EQ(result.count(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}