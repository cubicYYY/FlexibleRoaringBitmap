#include <gtest/gtest.h>

#include "froaring.h"

using namespace froaring;

class FlexibleRoaringIteratorTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FlexibleRoaringIteratorTest, BeginEndTest) {
    FlexibleRoaring<uint32_t, 16, 16> bitmap;
    auto it = FlexibleRoaringIterator<uint32_t, 16, 16>::begin(bitmap);
    auto end = FlexibleRoaringIterator<uint32_t, 16, 16>::end(bitmap);
    EXPECT_EQ(it, end);
}

TEST_F(FlexibleRoaringIteratorTest, SingleElementTest) {
    FlexibleRoaring<uint32_t, 16, 16> bitmap;
    bitmap.set(1);
    auto it = FlexibleRoaringIterator<uint32_t, 16, 16>::begin(bitmap);
    auto end = FlexibleRoaringIterator<uint32_t, 16, 16>::end(bitmap);
    EXPECT_NE(it, end);
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(it, end);
}

TEST_F(FlexibleRoaringIteratorTest, MultipleElementsTest) {
    FlexibleRoaring<uint32_t, 16, 16> bitmap;
    bitmap.set(1);
    bitmap.set(2);
    bitmap.set(3);
    auto it = FlexibleRoaringIterator<uint32_t, 16, 16>::begin(bitmap);
    auto end = FlexibleRoaringIterator<uint32_t, 16, 16>::end(bitmap);
    EXPECT_NE(it, end);
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_NE(it, end);
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_NE(it, end);
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, end);
}

TEST_F(FlexibleRoaringIteratorTest, ContainersTest) {
    FlexibleRoaring<uint32_t, 16, 16> bitmap;
    for (uint32_t i = 0; i < 100; ++i) {
        bitmap.set(i);
    }
    auto it = FlexibleRoaringIterator<uint32_t, 16, 16>::begin(bitmap);
    auto end = FlexibleRoaringIterator<uint32_t, 16, 16>::end(bitmap);
    for (uint32_t i = 0; i < 100; ++i) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, i);
        ++it;
    }
    EXPECT_EQ(it, end);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}