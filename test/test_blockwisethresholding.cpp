#include <iostream>

#include <bw/sourcehdf5.h>
#include <bw/sinkhdf5.h>
#include <bw/thresholding.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct ThresholdingTest {
void test() {
    using namespace vigra;
    typedef Thresholding<3, float>::V V;
   
    MultiArray<3, float> data(V(24,33,40));
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
    {
        HDF5File f("test.h5", HDF5File::Open);
        f.write("test", data);
    }
    
    SourceHDF5<3, float> source("test.h5", "test");
    SinkHDF5<3, vigra::UInt8> sink("thresh.h5", "thresh");
    sink.setBlockShape(V(10,10,10));
    
    Thresholding<3, float> bs(&source, V(6,4,7));
    
    bs.run(0.5, 0, 1, &sink);
   
    HDF5File f("thresh.h5", HDF5File::Open);
    MultiArray<3, UInt8> r;
    f.readAndResize("thresh", r);
   
    MultiArray<3, UInt8> t(data.shape());
    transformMultiArray(srcMultiArrayRange(data), destMultiArray(t), Threshold<float, UInt8>(-std::numeric_limits<float>::max(), 0.5, 1, 0));
    shouldEqual(t.shape(), r.shape());
    shouldEqualSequence(r.begin(), r.end(), t.begin());
}
}; /* struct ThresholdingTest */

struct ThresholdingTestSuite : public vigra::test_suite {
    ThresholdingTestSuite()
        : vigra::test_suite("ThresholdingTestSuite")
    {
        add( testCase(&ThresholdingTest::test) );
    }
};

int main(int argc, char ** argv) {
    ThresholdingTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
