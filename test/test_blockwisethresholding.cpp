#include <iostream>

#include "blockwisecc.h"
#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

struct BlockwiseThresholdingTest {
void test() {
    using namespace vigra;
    typedef typename BlockwiseThresholding<3, float>::V V;
   
    MultiArray<3, float> data(V(10,20,30));
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
    {
        HDF5File f("test.h5", HDF5File::Open);
        f.write("test", data);
    }
    
    BlockwiseThresholding<3, float> bs("test.h5", "test", V(10,10,10));
    bs.run(0.5, 0, 1, "thresh.h5", "thresh");
   
    HDF5File f("thresh.h5", HDF5File::Open);
    MultiArray<3, UInt8> r;
    f.readAndResize("thresh", r);
   
    MultiArray<3, UInt8> t(data.shape());
    transformMultiArray(srcMultiArrayRange(data), destMultiArray(t), Threshold<float, UInt8>(-std::numeric_limits<float>::max(), 0.5, 1, 0));
    shouldEqual(t.shape(), r.shape());
    shouldEqualSequence(r.begin(), r.end(), t.begin());
}
}; /* struct BlockwiseThresholdingTest */

struct BlockwiseThresholdingTestSuite : public vigra::test_suite {
    BlockwiseThresholdingTestSuite()
        : vigra::test_suite("BlockwiseThresholdingTestSuite")
    {
        add( testCase(&BlockwiseThresholdingTest::test) );
    }
};

int main(int argc, char ** argv) {
    BlockwiseThresholdingTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
