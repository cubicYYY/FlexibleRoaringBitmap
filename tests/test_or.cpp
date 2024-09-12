#include <gtest/gtest.h>

#include "froaring.h"

namespace froaring {
class FroaringOrTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FroaringOrTest, OrOperatorBothUninitialized) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    auto result = a | b;
    ASSERT_FALSE(result.is_inited());
}

TEST_F(FroaringOrTest, OrOperatorLeftUninitialized) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    b.set(1);
    auto result = a | b;
    result.debug_print();
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
}

TEST_F(FroaringOrTest, OrOperatorRightUninitialized) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    a.set(1);
    auto result = a | b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
}

TEST_F(FroaringOrTest, OrOperatorBothSingleContainer) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    a.set(1);
    b.set(2);
    ASSERT_TRUE(a.handle.type == CTy::Array);
    ASSERT_TRUE(b.handle.type == CTy::Array);
    auto result = a | b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
    ASSERT_TRUE(result.test(2));
}

TEST_F(FroaringOrTest, OrOperatorBothContainers) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    a.set(1);
    a.set(10000);
    b.set(2);
    b.set(10000);
    auto result = a | b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
    ASSERT_TRUE(result.test(2));
    ASSERT_TRUE(result.test(10000));
}

TEST_F(FroaringOrTest, OrOperatorOneContainer) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    a.set(1);
    a.set(10000);
    b.set(2);
    auto result = a | b;
    ASSERT_TRUE(result.is_inited());
    ASSERT_TRUE(result.test(1));
    ASSERT_TRUE(result.test(2));
    ASSERT_TRUE(result.test(10000));
}

TEST_F(FroaringOrTest, OrOperatorRangeTest) {
    FlexibleRoaringBitmap<uint64_t, 16, 8> a;
    FlexibleRoaringBitmap<uint64_t, 16, 8> b;
    for (uint64_t i = 200; i <= 260; ++i) {
        a.set(i);
    }
    for (uint64_t i = 263; i <= 513; ++i) {
        b.set(i);
    }
    auto result = a | b;
    result.debug_print();
    ASSERT_TRUE(result.is_inited());
    for (uint64_t i = 200; i < 260; ++i) {
        ASSERT_TRUE(result.test(i));
    }
    for (uint64_t i = 263; i < 513; ++i) {
        ASSERT_TRUE(result.test(i));
    }
    ASSERT_FALSE(result.test(261));
    ASSERT_FALSE(result.test(262));
    ASSERT_TRUE(result.test(260));
    ASSERT_TRUE(result.test(513));
}

}  // namespace froaring

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}