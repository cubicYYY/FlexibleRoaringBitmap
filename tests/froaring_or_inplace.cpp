#include <gtest/gtest.h>

#include "froaring.h"

namespace froaring {
class FroaringOrTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FroaringOrTest, OrOperatorBothUninitialized) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    a |= b;
    EXPECT_FALSE(a.is_inited());
}

TEST_F(FroaringOrTest, OrOperatorLeftUninitialized) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    b.set(1);
    a |= b;
    a.debug_print();
    EXPECT_TRUE(a.is_inited());
    EXPECT_TRUE(a.test(1));
}

TEST_F(FroaringOrTest, OrOperatorRightUninitialized) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    a.set(1);
    a |= b;
    EXPECT_TRUE(a.is_inited());
    EXPECT_TRUE(a.test(1));
}

TEST_F(FroaringOrTest, OrOperatorBothSingleContainer) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    a.set(1);
    b.set(2);
    EXPECT_TRUE(a.handle.type == CTy::Array);
    EXPECT_TRUE(b.handle.type == CTy::Array);
    a |= b;
    EXPECT_TRUE(a.is_inited());
    EXPECT_TRUE(a.test(1));
    EXPECT_TRUE(a.test(2));
}

TEST_F(FroaringOrTest, OrOperatorBothContainers) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    a.set(1);
    a.set(10000);
    b.set(2);
    b.set(10000);
    a |= b;
    EXPECT_TRUE(a.is_inited());
    EXPECT_TRUE(a.test(1));
    EXPECT_TRUE(a.test(2));
    EXPECT_TRUE(a.test(10000));
}

TEST_F(FroaringOrTest, OrOperatorOneContainer) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    a.set(1);
    a.set(10000);
    b.set(2);
    a |= b;
    EXPECT_TRUE(a.is_inited());
    EXPECT_TRUE(a.test(1));
    EXPECT_TRUE(a.test(2));
    EXPECT_TRUE(a.test(10000));
}

TEST_F(FroaringOrTest, OrOperatorRangeTest) {
    FlexibleRoaring<uint64_t, 16, 8> a;
    FlexibleRoaring<uint64_t, 16, 8> b;
    for (uint64_t i = 200; i <= 260; ++i) {
        a.set(i);
    }
    for (uint64_t i = 263; i <= 513; ++i) {
        b.set(i);
    }
    EXPECT_EQ(a.handle.type, ContainerType::Containers);
    EXPECT_EQ(b.handle.type, ContainerType::Containers);
    a |= b;
    a.debug_print();
    EXPECT_EQ(a.handle.type, ContainerType::Containers);
    EXPECT_TRUE(a.is_inited());
    for (uint64_t i = 200; i < 260; ++i) {
        EXPECT_TRUE(a.test(i));
    }
    for (uint64_t i = 263; i < 513; ++i) {
        EXPECT_TRUE(a.test(i));
    }
    EXPECT_FALSE(a.test(261));
    EXPECT_FALSE(a.test(262));
    EXPECT_TRUE(a.test(260));
    EXPECT_TRUE(a.test(513));
}

}  // namespace froaring

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}