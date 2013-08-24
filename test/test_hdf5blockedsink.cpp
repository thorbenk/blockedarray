#include <iostream>

#include <bw/sinkhdf5.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

using namespace BW;

struct SinkHDF5Test {
void test() {
    using namespace vigra;
    typedef typename SinkHDF5<3, float>::V V;
   
    MultiArray<3, float> data(V(10,20,30));
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
  
    SinkHDF5<3, float> bs("test.h5", "test", 1);
    bs.setShape(data.shape());
    bs.setBlockShape(V(10,10,10));
    bs.writeBlock(Roi<3>(V(), data.shape()), data);
   
    HDF5File f("test.h5", HDF5File::Open);
    MultiArray<3, float> r;
    f.readAndResize("test", r);
    shouldEqual(r.shape(), data.shape());
    shouldEqualSequence(data.begin(), data.end(), r.begin());
}
}; /* struct SinkHDF5Test */

struct SinkHDF5TestSuite : public vigra::test_suite {
    SinkHDF5TestSuite()
        : vigra::test_suite("SinkHDF5TestSuite")
    {
        add( testCase(&SinkHDF5Test::test) );
    }
};

int main(int argc, char ** argv) {
    SinkHDF5TestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
