#include <gtest/gtest.h>

#include "froaring_api/and.h"

using namespace froaring;

class FroaringAndTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FroaringAndTest, AndBitmapBitmap) {
    BitmapContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_and_bb(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringAndTest, AndArrayArray) {
    ArrayContainer<uint32_t, 16> a;
    ArrayContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_and_aa(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Array);
    EXPECT_EQ((static_cast<ArrayContainer<uint32_t, 16>*>(result))->size, 1);
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringAndTest, AndBitmapArray) {
    BitmapContainer<uint32_t, 16> a;
    ArrayContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_and_ba(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Array);
    EXPECT_EQ((static_cast<ArrayContainer<uint32_t, 16>*>(result))->size, 1);
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringAndTest, AndArrayBitmap) {
    ArrayContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);

    CTy result_type;
    auto* result = froaring_and_ba(&b, &a, result_type);
    EXPECT_EQ(result_type, CTy::Array);
    EXPECT_EQ((static_cast<ArrayContainer<uint32_t, 16>*>(result))->size, 1);
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
    delete result;
}

TEST_F(FroaringAndTest, AndBitmapRLE) {
    BitmapContainer<uint32_t, 16> a;
    RLEContainer<uint32_t, 16> b;
    a.set(1);
    a.set(2);
    for (uint32_t i = 2; i <= 4; ++i) {
        b.set(i);
    }

    CTy result_type;
    auto* result = froaring_and_br(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Array);
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(4));
    delete result;
}

TEST_F(FroaringAndTest, AndRLEBitmap) {
    RLEContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    for (uint32_t i = 1; i <= 3; ++i) {
        a.set(i);
    }
    b.set(2);
    b.set(3);
    b.set(4);

    CTy result_type;
    auto* result = froaring_and_br(&b, &a, result_type);
    EXPECT_EQ(result_type, CTy::Array);
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(1));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(2));
    EXPECT_TRUE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(3));
    EXPECT_FALSE((static_cast<ArrayContainer<uint32_t, 16>*>(result))->test(4));
    delete result;
}

TEST_F(FroaringAndTest, AndRangeTest) {
    BitmapContainer<uint32_t, 16> a;
    BitmapContainer<uint32_t, 16> b;
    for (uint32_t i = 200; i <= 260; ++i) {
        a.set(i);
    }
    for (uint32_t i = 250; i <= 300; ++i) {
        b.set(i);
    }

    CTy result_type;
    auto* result = froaring_and_bb(&a, &b, result_type);
    EXPECT_EQ(result_type, CTy::Bitmap);
    for (uint32_t i = 200; i < 250; ++i) {
        EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    for (uint32_t i = 250; i <= 260; ++i) {
        EXPECT_TRUE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    for (uint32_t i = 261; i <= 300; ++i) {
        EXPECT_FALSE((static_cast<BitmapContainer<uint32_t, 16>*>(result))->test(i));
    }
    delete result;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}