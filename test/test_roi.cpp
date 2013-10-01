/************************************************************************/
/*                                                                      */
/*    Copyright 2013 by Thorben Kroeger                                 */
/*    thorben.kroeger@iwr.uni-heidelberg.de                             */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

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
