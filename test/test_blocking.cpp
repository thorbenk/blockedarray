#include <iostream>

#include "blockwisecc.h"
#include "test_utils.h"

#include <vigra/unittest.hxx>

struct BlockingTest {
void testConstruction() {
    typedef Blocking<2>::V V;
   
    Blocking<2> bb(Roi<2>(V(0,0), V(100,200)), V(50,25), V());
    shouldEqual(bb.numBlocks(), 2*8);
}
}; /* struct BlockingTest */

struct BlockingTestSuite : public vigra::test_suite {
    BlockingTestSuite()
        : vigra::test_suite("BlockingTestSuite")
    {
        add( testCase(&BlockingTest::testConstruction));
    }
};

int main(int argc, char ** argv) {
    BlockingTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
