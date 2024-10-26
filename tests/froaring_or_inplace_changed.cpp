#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

class FlexibleRoaringTest : public ::testing::Test {
protected:
    FlexibleRoaring<uint64_t, 16, 8> bitmap1;
    FlexibleRoaring<uint64_t, 16, 8> bitmap2;

    void SetUp() override {
        bitmap1.set(1);
        bitmap1.set(2);
        bitmap2.set(2);
        bitmap2.set(3);
    }
};

TEST_F(FlexibleRoaringTest, TestChangedWhenBothEmpty) {
    FlexibleRoaring<uint64_t, 16, 8> empty_bitmap1;
    FlexibleRoaring<uint64_t, 16, 8> empty_bitmap2;
    bool changed = empty_bitmap1 |= empty_bitmap2;
    EXPECT_FALSE(changed);
}

TEST_F(FlexibleRoaringTest, TestChangedWhenOneEmpty) {
    FlexibleRoaring<uint64_t, 16, 8> empty_bitmap;
    bool changed = bitmap1 |= empty_bitmap;
    EXPECT_FALSE(changed);
}

TEST_F(FlexibleRoaringTest, TestChangedWhenBothNonEmpty) {
    bool changed = bitmap1 |= bitmap2;
    EXPECT_TRUE(changed);
    EXPECT_TRUE(bitmap1.test(1));
    EXPECT_TRUE(bitmap1.test(2));
    EXPECT_TRUE(bitmap1.test(3));
}

TEST_F(FlexibleRoaringTest, TestChangedWhenNoChange) {
    FlexibleRoaring<uint64_t, 16, 8> bitmap3;
    bitmap3.set(1);
    bitmap3.set(2);
    bitmap1.debug_print();
    bitmap3.debug_print();
    bool changed = bitmap1 |= bitmap3;
    bitmap1.debug_print();
    EXPECT_FALSE(changed);
    EXPECT_TRUE(bitmap1.test(1));
    EXPECT_TRUE(bitmap1.test(2));
    EXPECT_FALSE(bitmap1.test(3));
}

TEST_F(FlexibleRoaringTest, TestChangedWithDifferentIndexes) {
    FlexibleRoaring<uint64_t, 16, 8> bitmap3;
    bitmap3.set(1 << 16);  // Different index
    bool changed = bitmap1 |= bitmap3;
    EXPECT_TRUE(changed);
    EXPECT_TRUE(bitmap1.test(1));
    EXPECT_TRUE(bitmap1.test(2));
    EXPECT_TRUE(bitmap1.test(1 << 16));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}