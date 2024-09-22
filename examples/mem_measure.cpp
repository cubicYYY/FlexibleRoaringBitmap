#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "froaring.h"

using namespace std;

#define COUNT 100000
int main() {
    froaring::FlexibleRoaring<> a[COUNT];
    // while (1) {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
}