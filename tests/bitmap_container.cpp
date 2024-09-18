#include "bitmap_container.h"

#include <gtest/gtest.h>

namespace froaring {
class BitmapContainerTest : public ::testing::Test {
protected:
    BitmapContainer<uint64_t, 8>* container;

    void SetUp() override { container = new BitmapContainer<uint64_t, 8>(); }
    void TearDown() override { delete container; }
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

TEST_F(BitmapContainerTest, NonPowerOf2SizeBitmap) {
    auto* container = new BitmapContainer<uint64_t, 10>();
    container->set(909);
    container->set(910);
    container->set(911);
    container->set(912);
    container->set(913);

    container->set(1023);

    container->set(962);
    container->set(963);
    container->set(964);
    container->set(965);
    container->set(966);
    container->set(967);
    container->set(968);
    container->reset(967);
    EXPECT_EQ(container->IndexInsideWordMask, 0x3F);  // uint64_t, 64 bits = 2^6 bits, (111111)2=0x3F
    container->debug_print();
    EXPECT_TRUE(container->test(1023));
    EXPECT_TRUE(container->containesRange(909, 913));
    EXPECT_FALSE(container->containesRange(909, 914));
    EXPECT_FALSE(container->containesRange(908, 913));
    EXPECT_TRUE(container->containesRange(962, 966));
    EXPECT_TRUE(container->containesRange(968, 968));
    EXPECT_FALSE(container->containesRange(962, 968));

    EXPECT_EQ(container->cardinality(), 12);
    delete container;
}

TEST_F(BitmapContainerTest, ResetRange) {
    auto* container = new BitmapContainer<uint64_t, 10>();
    container->set(909);
    container->set(910);
    container->set(911);
    container->set(912);
    container->set(913);

    container->reset_range(910, 912);
    EXPECT_TRUE(container->test(909));
    EXPECT_FALSE(container->test(910));
    EXPECT_FALSE(container->test(911));
    EXPECT_FALSE(container->test(912));
    EXPECT_TRUE(container->test(913));

    delete container;
}

}  // namespace froaring
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}