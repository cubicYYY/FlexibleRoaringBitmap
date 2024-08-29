#include <gtest/gtest.h>

#include "rle_container.h"

namespace froaring {

class RLEContainerTest : public ::testing::Test {
protected:
    using RLESized = RLEContainer<uint64_t, 8>;
    RLEContainer<uint64_t, 8>* container;

    void SetUp() override { container = RLEContainer<uint64_t, 8>::create(); }
};

TEST_F(RLEContainerTest, SetSingleValue) {
    RLESized::set(container, 5);
    EXPECT_TRUE(container->test(5));
    EXPECT_EQ(container->cardinality(), 1);
}

TEST_F(RLEContainerTest, SetMultipleValues) {
    RLESized::set(container, 5);
    RLESized::set(container, 10);
    RLESized::set(container, 15);
    EXPECT_TRUE(container->test(5));
    EXPECT_TRUE(container->test(10));
    EXPECT_TRUE(container->test(15));
    EXPECT_EQ(container->cardinality(), 3);
}

TEST_F(RLEContainerTest, SetAndMergeRuns) {
    RLESized::set(container, 5);
    RLESized::set(container, 6);
    RLESized::set(container, 7);
    EXPECT_TRUE(container->test(5));
    EXPECT_TRUE(container->test(6));
    EXPECT_TRUE(container->test(7));
    EXPECT_EQ(container->cardinality(), 3);
}

TEST_F(RLEContainerTest, ResetSingleValue) {
    RLESized::set(container, 5);
    RLESized::reset(container, 5);
    EXPECT_FALSE(container->test(5));
    EXPECT_EQ(container->cardinality(), 0);
}

TEST_F(RLEContainerTest, ResetValueInRun) {
    RLESized::set(container, 5);
    RLESized::set(container, 6);
    RLESized::set(container, 7);
    RLESized::set(container, 7);
    RLESized::set(container, 8);
    RLESized::set(container, 9);

    RLESized::set(container, 255);
    RLESized::set(container, 254);
    RLESized::set(container, 253);
    RLESized::set(container, 252);
    RLESized::set(container, 251);

    RLESized::reset(container, 7);
    RLESized::reset(container, 253);

    EXPECT_EQ(container->pairs(), 4);
    EXPECT_TRUE(container->test(5));
    EXPECT_TRUE(container->test(6));
    EXPECT_FALSE(container->test(7));
    EXPECT_EQ(container->cardinality(), 8);
}

TEST_F(RLEContainerTest, ResetAndSplitRun) {
    RLESized::set(container, 5);
    RLESized::set(container, 6);
    RLESized::set(container, 7);
    RLESized::reset(container, 6);
    EXPECT_TRUE(container->test(5));
    EXPECT_FALSE(container->test(6));
    EXPECT_TRUE(container->test(7));
    EXPECT_EQ(container->cardinality(), 2);
}

TEST_F(RLEContainerTest, AlternativelySetTest) {
    for (size_t i = 0; i < 256; i += 2) {
        RLESized::set(container, i);
    }
    EXPECT_EQ(container->cardinality(), 128);
    EXPECT_EQ(container->pairs(), 128);
    for (size_t i = 1; i < 256; i += 2) {
        RLESized::set(container, i);
    }
    EXPECT_EQ(container->cardinality(), 256);
    EXPECT_EQ(container->pairs(), 1);
}

}  // namespace froaring

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}