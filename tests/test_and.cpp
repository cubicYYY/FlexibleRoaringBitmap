#include <gtest/gtest.h>
#include "froaring.h"

using namespace froaring;

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
    bitmap1.debug_print();
    bitmap2.debug_print();
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

    bitmap1.debug_print();
    bitmap2.debug_print();
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

TEST(FlexibleRoaringBitmapTest, AndContainersConvertSingle) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    EXPECT_EQ(bitmap1.is_inited(), false);
    EXPECT_EQ(bitmap2.is_inited(), false);
    bitmap1.set(114514);
    bitmap1.set(999);

    bitmap2.set(114514);
    std::cout << "Type1:" << static_cast<int>(bitmap1.handle.type) << std::endl;
    std::cout << "Index1:" << bitmap1.handle.index << std::endl;
    std::cout << "Type1:" << static_cast<int>(bitmap2.handle.type) << std::endl;
    std::cout << "Index1:" << bitmap2.handle.index << std::endl;
    EXPECT_EQ(bitmap1.is_inited(), true);
    EXPECT_EQ(bitmap2.is_inited(), true);
    EXPECT_EQ(bitmap2.handle.index, 447);
    bitmap1.debug_print();
    bitmap2.debug_print();

    auto result = bitmap1 & bitmap2;
    result.debug_print();
    EXPECT_TRUE(result.is_inited());
    EXPECT_EQ(result.count(), 1);
    EXPECT_EQ(result.handle.type, CTy::Array);
}

TEST(FlexibleRoaringBitmapTest, AndMixed) {
    FlexibleRoaringBitmap bitmap1;
    FlexibleRoaringBitmap bitmap2;
    bitmap1.set(1919810);
    bitmap1.set(114514);
    bitmap1.set(999);

    bitmap2.set(999);
    bitmap2.set(773773);
    auto result = bitmap1 & bitmap2;
    result.debug_print();
    EXPECT_TRUE(result.is_inited());
    EXPECT_TRUE(result.test(999));
    EXPECT_EQ(result.count(), 1);
    // EXPECT_EQ(result.handle.type, CTy::Array);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}