#include <gtest/gtest.h>

#include "bitmap_container.h"

namespace froaring {
class BitmapContainerTest : public ::testing::Test {
protected:
    BitmapContainer<uint64_t, 8>* container;

    void SetUp() override {
        container = BitmapContainer<uint64_t, 8>::create();
    }
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

    EXPECT_EQ(container->cardinality(), 6);
    container->reset(11);
    EXPECT_EQ(container->cardinality(), 5);
}

TEST_F(BitmapContainerTest, TestTestAndSet) {
    EXPECT_TRUE(container->test_and_set(255));
    EXPECT_TRUE(container->test(255));
    EXPECT_FALSE(container->test_and_set(255));
}

}  // namespace froaring
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}