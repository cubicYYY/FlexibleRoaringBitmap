#include <iostream>
#include <vector>

#include "froaring.h"

using namespace std;

int main() {
    froaring::FlexibleRoaringBitmap<> a;

    a.set(1);
    a.set(100);
    a.set(101);
    assert(a.test(1));
    assert(a.test(100));
    assert(a.test(101));
    assert(!a.test(99));
    assert(!a.test(102));
    assert(!a.test(102));

    a.set(102);
    assert(a.test(102));

    a.set(114514);
    assert(a.test(114514));
    assert(!a.test(114513));
    assert(!a.test(114515));

    a.reset(114514);
    assert(!a.test(114514));
    assert(!a.test(114513));
    assert(!a.test(114515));

    cout << a.count() << endl;
    assert(a.count() == 4);
    cout << (a == a) << endl;
    cout << sizeof(a) << endl;
    cout << sizeof(std::vector<int>) << endl;
    cout << sizeof(size_t) << endl;
}