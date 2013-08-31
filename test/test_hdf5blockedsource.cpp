#include <iostream>

#include <bw/sourcehdf5.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct SourceHDF5Test {
void test() {
    using namespace vigra;
    typedef typename SourceHDF5<3, float>::V V;
   
    HDF5File f("test.h5", HDF5File::Open);
    MultiArray<3, float> data(V(10,20,30));
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
    f.write<3, float>("test", data);
    f.close();
   
    SourceHDF5<3, float> bs("test.h5", "test");
    shouldEqual(bs.shape(), V(10,20,30));
   
    MultiArray<3, float> r(data.shape());
    bool ret = bs.readBlock(Roi<3>(V(), data.shape()), r);
    should(ret);
    shouldEqualSequence(data.begin(), data.end(), r.begin());
}
}; /* struct SourceHDF5Test */

struct SourceHDF5TestSuite : public vigra::test_suite {
    SourceHDF5TestSuite()
        : vigra::test_suite("SourceHDF5TestSuite")
    {
        add( testCase(&SourceHDF5Test::test) );
    }
};

int main(int argc, char ** argv) {
    SourceHDF5TestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
