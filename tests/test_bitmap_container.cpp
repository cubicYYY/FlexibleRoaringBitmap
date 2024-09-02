#include <gtest/gtest.h>

#include "bitmap_container.h"

namespace froaring {
class BitmapContainerTest : public ::testing::Test {
protected:
    BitmapContainer<uint64_t, 8>* container;

    void SetUp() override { container = new BitmapContainer<uint64_t, 8>(); }
};

TEST_F(BitmapContainerTest, TestSetAndTest) {
    container->set(5);
    EXPECT_TRUE(container->test(5));
    EXPECT_FALSE(container->test(10));
}

TEST_F(BitmapContainerTest, TestReset) {
    container->set(5);
    container->reset(5);
    EXPECT_FALSE(container->test(5));
}

TEST_F(BitmapContainerTest, TestCardinality) {
    container->set(5);
    container->set(10);
    container->set(11);
    container->set(11);
    container->set(12);
    container->set(255);
    container->set(254);
    container->debug_print();
    EXPECT_EQ(container->cardinality(), 6);
    container->reset(11);
    EXPECT_EQ(container->cardinality(), 5);
}

TEST_F(BitmapContainerTest, TestTestAndSet) {
    EXPECT_TRUE(container->test_and_set(255));
    EXPECT_TRUE(container->test(255));
    EXPECT_FALSE(container->test_and_set(255));
}

TEST_F(BitmapContainerTest, ContainsRangeTest) {
    container->set(9);
    container->set(10);
    container->set(11);
    container->set(12);
    container->set(13);

    container->set(255);

    container->set(62);
    container->set(63);
    container->set(64);
    container->set(65);
    container->set(66);
    container->set(67);
    container->set(68);
    container->reset(67);
    EXPECT_EQ(container->IndexInsideWordMask, 0x3F);  // uint64_t, 64 bits = 2^6 bits, (111111)2=0x3F
    container->debug_print();
    EXPECT_TRUE(container->containesRange(9, 13));
    EXPECT_FALSE(container->containesRange(9, 14));
    EXPECT_FALSE(container->containesRange(8, 13));
    EXPECT_TRUE(container->containesRange(62, 66));
    EXPECT_TRUE(container->containesRange(68, 68));
    EXPECT_FALSE(container->containesRange(62, 68));

    EXPECT_EQ(container->cardinality(), 12);
}

}  // namespace froaring
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}