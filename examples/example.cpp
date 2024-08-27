#include <iostream>

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
    assert(a.cardinality() == 3);

    cout << sizeof(a) << endl;
}