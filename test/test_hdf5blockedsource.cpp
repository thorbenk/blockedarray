#include <iostream>

#include "blockwisecc.h"
#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

struct HDF5BlockedSourceTest {
void test() {
    using namespace vigra;
    typedef typename HDF5BlockedSource<3, float>::V V;
   
    HDF5File f("test.h5", HDF5File::Open);
    f.createDataset<3, float>("test", V(10,20,30), 0);
    f.close();
   
    HDF5BlockedSource<3, float> bs("test.h5", "test");
    shouldEqual(bs.shape(), V(10,20,30));
    
}
}; /* struct HDF5BlockedSourceTest */

struct HDF5BlockedSourceTestSuite : public vigra::test_suite {
    HDF5BlockedSourceTestSuite()
        : vigra::test_suite("HDF5BlockedSourceTestSuite")
    {
        add( testCase(&HDF5BlockedSourceTest::test) );
    }
};

int main(int argc, char ** argv) {
    HDF5BlockedSourceTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
