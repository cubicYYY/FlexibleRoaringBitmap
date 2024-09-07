#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#include "froaring.h"

using namespace std;

#define COUNT 100000
int main() {
    froaring::FlexibleRoaringBitmap<> a[COUNT];
    // while (1) {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
}