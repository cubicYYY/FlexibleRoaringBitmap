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
    assert(a == a);

    froaring::FlexibleRoaringBitmap<> x, y;
    x.set(1);
    x.set(2);
    x.set(3);
    x.set(114514);
    x.set(1919810);
    x.reset(2);
    y.set(1);
    y.set(3);
    y.set(114514);
    y.set(1919810);
    assert(x == y);

    cout << (a == a) << endl;
    cout << sizeof(a) << endl;
    cout << sizeof(std::vector<int>) << endl;
    cout << sizeof(size_t) << endl;
}