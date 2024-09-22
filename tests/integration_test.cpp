#include <gtest/gtest.h>

#include "froaring.h"

class FlexibleRoaringTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};
namespace froaring {
TEST(FlexibleRoaringTest, RemoveAndCheckEmptyContainers) {
    FlexibleRoaring<uint64_t, 16, 8> bitmap;

    bitmap.set(1);
    bitmap.set(2);
    bitmap.set(3);
    bitmap.debug_print();
    EXPECT_TRUE(bitmap.test(1));
    EXPECT_TRUE(bitmap.test(2));
    EXPECT_TRUE(bitmap.test(3));
    std::cout << "RESET:" << std::endl;
    bitmap.reset(3);
    std::cout << "SET:" << std::endl;
    bitmap.set(114514);
    std::cout << "RESET:" << std::endl;
    bitmap.reset(2);
    std::cout << "SET:" << std::endl;
    bitmap.set(1919810);
    std::cout << "RESET:" << std::endl;
    bitmap.reset(1);
    std::cout << "PRINT:" << std::endl;
    bitmap.debug_print();
    std::cout << "TEST:" << std::endl;
    EXPECT_FALSE(bitmap.test(1));
    EXPECT_FALSE(bitmap.test(2));
    EXPECT_FALSE(bitmap.test(3));
    EXPECT_FALSE(bitmap.test(1919809));
    EXPECT_FALSE(bitmap.test(1919811));
    EXPECT_TRUE(bitmap.test(1919810));

    EXPECT_EQ(bitmap.count(), 2);
    bitmap.debug_print();

    std::cout << "Finished..." << std::endl;
}

TEST(FlexibleRoaringTest, TypeConversion) {
    FlexibleRoaring<uint64_t, 16, 8> bitmap;

    for (auto i = 0; i < 1000; i++) {
        bitmap.set(i);
        bitmap.set(i + 4090);
    }
    EXPECT_EQ(bitmap.count(), 2000);

    for (auto i = 0; i < 1000; i++) {
        bitmap.set(i);
        bitmap.set(i + 4090);
    }

    EXPECT_EQ(bitmap.count(), 2000);

    for (auto i = 0; i < 1000; i++) {
        bitmap.reset(i);
        bitmap.reset(i + 4090);
    }

    EXPECT_EQ(bitmap.count(), 0);
}
}  // namespace froaring

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}