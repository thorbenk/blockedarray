#include <iostream>

#include "hdf5blockedsink.h"
#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

struct HDF5BlockedSinkTest {
void test() {
    using namespace vigra;
    typedef typename HDF5BlockedSink<3, float>::V V;
   
    MultiArray<3, float> data(V(10,20,30));
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
  
    HDF5BlockedSink<3, float> bs("test.h5", "test", 1);
    bs.setShape(data.shape());
    bs.setBlockShape(V(10,10,10));
    bs.writeBlock(Roi<3>(V(), data.shape()), data);
   
    HDF5File f("test.h5", HDF5File::Open);
    MultiArray<3, float> r;
    f.readAndResize("test", r);
    shouldEqual(r.shape(), data.shape());
    shouldEqualSequence(data.begin(), data.end(), r.begin());
}
}; /* struct HDF5BlockedSinkTest */

struct HDF5BlockedSinkTestSuite : public vigra::test_suite {
    HDF5BlockedSinkTestSuite()
        : vigra::test_suite("HDF5BlockedSinkTestSuite")
    {
        add( testCase(&HDF5BlockedSinkTest::test) );
    }
};

int main(int argc, char ** argv) {
    HDF5BlockedSinkTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
