#include <iostream>

#include <bw/roi.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct RoiTest {
void testConstruction() {
    typedef Roi<3>::V V;
    {
        Roi<3> r(V(2,3,4), V(5,6,8));
        shouldEqual(r.p, V(2,3,4));
        shouldEqual(r.q, V(5,6,8));
        shouldEqual(r.shape(), V(3,3,4));
    }
    {
        Roi<3> r;
        shouldEqual(r.p, V());
        shouldEqual(r.q, V());
        shouldEqual(r.shape(), V());
    }
}
void testIntersection() {
    typedef Roi<2>::V V;
    {
        Roi<2> r1(V(1,1), V(10,20));
        Roi<2> r2(V(5,6), V(30,40));
        Roi<2> out;
        should( r1.intersect(r2, out) );
        should( r2.intersect(r1, out) );
    }
    {
        Roi<2> r1(V(10,20), V(12,14));
        Roi<2> r2(V(15,18), V(30,40));
        Roi<2> out;
        should( !r1.intersect(r2, out) );
        should( !r2.intersect(r1, out) );
    }
}
void testAppendAxis() {
    typedef Roi<2>::V V2;
    typedef Roi<3>::V V3;
    Roi<2> r(V2(3,4), V2(7,9));
    Roi<3> s =r.appendAxis(10,13);
    shouldEqual(s, Roi<3>(V3(3,4,10), V3(7,9,13)));
}
void testRemoveAxis() {
    typedef Roi<2>::V V2;
    typedef Roi<3>::V V3;
    Roi<3> r(V3(3,4,10), V3(7,9,13));
    Roi<2> s = r.removeAxis(1);
    shouldEqual(s, Roi<2>(V2(3,10), V2(7,13)));
}
void testPrependAxis() {
    typedef Roi<2>::V V2;
    typedef Roi<3>::V V3;
    Roi<2> r(V2(3,4), V2(7,9));
    Roi<3> s =r.prependAxis(10,13);
    shouldEqual(s, Roi<3>(V3(10,3,4), V3(13,7,9)));
}
void testInsertAxisBefore() {
    typedef Roi<2>::V V2;
    typedef Roi<3>::V V3;
    Roi<2> r(V2(3,4), V2(7,9));
    Roi<3> s =r.insertAxisBefore(0,10,13);
    shouldEqual(s, Roi<3>(V3(10,3,4), V3(13,7,9)));
}
}; /* struct RoiTest */

struct RoiTestSuite : public vigra::test_suite {
    RoiTestSuite()
        : vigra::test_suite("RoiTestSuite")
    {
        add( testCase(&RoiTest::testConstruction));
        add( testCase(&RoiTest::testIntersection));
        add( testCase(&RoiTest::testAppendAxis));
        add( testCase(&RoiTest::testRemoveAxis));
        add( testCase(&RoiTest::testPrependAxis));
        add( testCase(&RoiTest::testInsertAxisBefore));
    }
};

int main(int argc, char ** argv) {
    RoiTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
