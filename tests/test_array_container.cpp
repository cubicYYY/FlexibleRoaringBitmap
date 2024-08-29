#include <gtest/gtest.h>

#include "array_container.h"

namespace froaring {

class ArrayContainerTest : public ::testing::Test {
protected:
    using ArrayContainerSized = ArrayContainer<uint64_t, 8>;
    ArrayContainerSized* container;

    void SetUp() override {
        container = ArrayContainerSized::create();
        container->clear();
        ArrayContainerSized::set(container, 1);
        ArrayContainerSized::set(container, 2);
        ArrayContainerSized::set(container, 3);
    }
};

TEST_F(ArrayContainerTest, SetBoundaryTest) {
    ArrayContainerSized::set(container, 0);
    EXPECT_TRUE(container->test(0));
    EXPECT_EQ(container->cardinality(), 4);

    ArrayContainerSized::set(container, 255);
    EXPECT_TRUE(container->test(255));
    EXPECT_EQ(container->cardinality(), 5);
}

TEST_F(ArrayContainerTest, ResetBoundaryTest) {
    ArrayContainerSized::set(container, 0);
    ArrayContainerSized::set(container, 255);

    container->reset(0);
    EXPECT_FALSE(container->test(0));
    EXPECT_EQ(container->cardinality(), 4);

    container->reset(255);
    EXPECT_FALSE(container->test(255));
    EXPECT_EQ(container->cardinality(), 3);
}

TEST_F(ArrayContainerTest, TestBoundaryTest) {
    ArrayContainerSized::set(container, 0);
    ArrayContainerSized::set(container, 255);

    EXPECT_TRUE(container->test(0));
    EXPECT_TRUE(255);
    EXPECT_EQ(container->cardinality(), 5);
}

TEST_F(ArrayContainerTest, ExpandBoundaryTest) {
    for (uint64_t i = 4; i <= 10; ++i) {
        ArrayContainerSized::set(container, i);
    }
    EXPECT_EQ(container->cardinality(), 10);
    EXPECT_TRUE(container->test(10));

    ArrayContainerSized::set(container, 255);
    EXPECT_EQ(container->cardinality(), 11);
}

TEST_F(ArrayContainerTest, ExpansionTest) {
    for (uint64_t i = 0; i <= 255; ++i) {
        ArrayContainerSized::set(container, i);
    }
    container->debug_print();
    EXPECT_EQ(container->cardinality(), 256);
    EXPECT_TRUE(container->test(255));
}

}  // namespace froaring

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}