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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}