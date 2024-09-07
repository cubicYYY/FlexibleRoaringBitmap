#include <gtest/gtest.h>

#include "rle_container.h"

namespace froaring {

class RLEContainerTest : public ::testing::Test {
protected:
    using RLESized = RLEContainer<uint64_t, 8>;
    RLESized* container;

    void SetUp() override { container = new RLESized(); }
    void TearDown() override { delete container; }
};

TEST_F(RLEContainerTest, SetSingleValue) {
    container->set(5);
    EXPECT_TRUE(container->test(5));
    EXPECT_EQ(container->cardinality(), 1);
}

TEST_F(RLEContainerTest, SetMultipleValues) {
    container->set(5);
    container->set(10);
    container->set(15);
    EXPECT_TRUE(container->test(5));
    EXPECT_TRUE(container->test(10));
    EXPECT_TRUE(container->test(15));
    EXPECT_EQ(container->cardinality(), 3);
}

TEST_F(RLEContainerTest, SetAndMergeRuns) {
    container->set(5);
    container->set(6);
    container->set(7);
    EXPECT_TRUE(container->test(5));
    EXPECT_TRUE(container->test(6));
    EXPECT_TRUE(container->test(7));
    EXPECT_EQ(container->cardinality(), 3);
}

TEST_F(RLEContainerTest, ResetSingleValue) {
    container->set(5);
    container->reset(5);
    EXPECT_FALSE(container->test(5));
    EXPECT_EQ(container->cardinality(), 0);
}

TEST_F(RLEContainerTest, ResetValueInRun) {
    container->set(5);
    container->set(6);
    container->set(7);
    container->set(7);
    container->set(8);
    container->set(9);

    container->set(255);
    container->set(254);
    container->set(253);
    container->set(252);
    container->set(251);

    container->reset(7);
    container->reset(253);

    EXPECT_EQ(container->runsCount(), 4);
    EXPECT_TRUE(container->test(5));
    EXPECT_TRUE(container->test(6));
    EXPECT_FALSE(container->test(7));
    EXPECT_EQ(container->cardinality(), 8);
}

TEST_F(RLEContainerTest, ResetAndSplitRun) {
    container->set(5);
    container->set(6);
    container->set(7);
    container->reset(6);
    EXPECT_TRUE(container->test(5));
    EXPECT_FALSE(container->test(6));
    EXPECT_TRUE(container->test(7));
    EXPECT_EQ(container->cardinality(), 2);
}

TEST_F(RLEContainerTest, AlternativelySetTest) {
    for (size_t i = 0; i < 256; i += 2) {
        container->set(i);
    }
    EXPECT_EQ(container->cardinality(), 128);
    EXPECT_EQ(container->runsCount(), 128);
    for (size_t i = 1; i < 256; i += 2) {
        container->set(i);
    }
    EXPECT_EQ(container->cardinality(), 256);
    EXPECT_EQ(container->runsCount(), 1);
}

}  // namespace froaring

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}