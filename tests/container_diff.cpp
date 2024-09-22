#include <gtest/gtest.h>

#include "froaring_api/diff.h"

using namespace froaring;

class FroaringDiffTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FroaringDiffTest, DiffBitmapBitmap) {
    BitmapContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_diff_bb(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringDiffTest, DiffArrayArray) {
    ArrayContainer<uint32_t, 16> a;
    ArrayContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_diff_aa(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Array);
    EXPECT_EQ((static_cast<ArrayContainer<uint32_t, 16>*>(result))->size, 1);
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringDiffTest, DiffBitmapArray) {
    BitmapContainer<uint32_t, 16> a;
    ArrayContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_diff_ba(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringDiffTest, DiffArrayBitmap) {
    ArrayContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_diff_ba(&b, &a, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    delete result;
}

TEST_F(FroaringDiffTest, DiffBitmapRLE) {
    BitmapContainer<uint32_t, 16> a;
    RLEContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.run_count = 1;
    b.runs[0] = {2, 4};

    CTy result_type;
    auto* result = froaring_diff_br(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(4));
    delete result;
}

TEST_F(FroaringDiffTest, DiffRLEBitmap) {
    RLEContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.run_count = 1;
    a.runs[0] = {1, 3};
    b.set(2);
    b.set(3);
    b.set(4);

    CTy result_type;
    auto* result = froaring_diff_br(&b, &a, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(4));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringDiffTest, DiffRangeTest) {
    BitmapContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    for (uint32_t i = 200; i <= 260; ++i) {
        a.set(i);
    }
    for (uint32_t i = 263; i <= 513; ++i) {
        b.set(i);
    }

    CTy result_type;
    auto* result = froaring_diff_bb(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    for (uint32_t i = 200; i <= 260; ++i) {
        EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    for (uint32_t i = 263; i <= 513; ++i) {
        EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(261));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(262));
    delete result;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}