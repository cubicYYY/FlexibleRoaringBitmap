#include <gtest/gtest.h>
#include "froaring_api/or_inplace.h"

using namespace froaring;

class FroaringOrInplaceTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(FroaringOrInplaceTest, OrInplaceBitmapBitmap) {
    BitmapContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_or_inplace_bb(&a, &b, result_type);
    ASSERT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
}

TEST_F(FroaringOrInplaceTest, OrInplaceArrayArray) {
    ArrayContainer<uint32_t, 16> a;
    ArrayContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_or_inplace_aa(&a, &b, result_type);
    ASSERT_EQ(result_type, CTy::Array);
    ASSERT_EQ((static_cast<ArrayContainer<uint32_t, 16>*>(result))->size, 3);
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
}

TEST_F(FroaringOrInplaceTest, OrInplaceBitmapArray) {
    BitmapContainer<uint32_t, 16> a;
    ArrayContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_or_inplace_ba(&a, &b, result_type);
    ASSERT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
}

TEST_F(FroaringOrInplaceTest, OrInplaceArrayBitmap) {
    ArrayContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_or_inplace_ab(&a, &b, result_type);
    ASSERT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
}

TEST_F(FroaringOrInplaceTest, OrInplaceBitmapRLE) {
    BitmapContainer<uint32_t, 16> a;
    RLEContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.runs[0] = {2,4};
    b.run_count = 1;
    CTy result_type;
    a.debug_print();
    auto* result = froaring_or_inplace_br(&a, &b, result_type);
    (static_cast<BitmapContainer<uint32_t, 16>*>(result))->debug_print();
    ASSERT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(4));
}

TEST_F(FroaringOrInplaceTest, OrInplaceRLEBitmap) {
    RLEContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.runs[0] = {1,3};
    a.run_count = 1;
    b.set(2);
    b.set(3);
    b.set(4);

    CTy result_type;
    auto* result = froaring_or_inplace_rb(&a, &b, result_type);
    ASSERT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(4));
}

TEST_F(FroaringOrInplaceTest, OrInplaceRangeTest) {
    BitmapContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    for (uint32_t i = 200; i <= 260; ++i) {
        a.set(i);
    }
    for (uint32_t i = 263; i <= 513; ++i) {
        b.set(i);
    }

    CTy result_type;
    auto* result = froaring_or_inplace_bb(&a, &b, result_type);
    ASSERT_EQ(result_type, CTy::Bitmap);
    for (uint32_t i = 200; i <= 260; ++i) {
        EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    for (uint32_t i = 263; i <= 513; ++i) {
        EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(261));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(262));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}