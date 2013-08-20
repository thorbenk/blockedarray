#include <iostream>

#include "blockwisecc.h"
#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

struct BlockwiseChannelSelectorTest {
void test() {
    using namespace vigra;
    typedef typename BlockwiseChannelSelector<4, float>::V V;
   
    for(int ch=0; ch<=1; ++ch) {
        std::cout << "* channel = " << ch << std::endl;
    
        MultiArray<4, float> data(vigra::TinyVector<int, 4>(10,20,30,2));
        FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
        {
            HDF5File f("test.h5", HDF5File::Open);
            f.write("test", data);
        }
        
        HDF5BlockedSource<4, float> source("test.h5", "test");
        HDF5BlockedSink<3, float> sink("channel.h5", "channel");
        sink.setBlockShape(V(10,10,10));
        
        BlockwiseChannelSelector<4, float> cs(&source, V(10,10,10));
        
        cs.run(3, ch, &sink);
    
        HDF5File f("channel.h5", HDF5File::Open);
        MultiArray<3, float> r;
        f.readAndResize("channel", r);
    
        MultiArrayView<3, float> shouldResult = data.bind<3>(ch);
        
        shouldEqualSequence(r.begin(), r.end(), shouldResult.begin());
    } //loop through channels
}
}; /* struct BlockwiseChannelSelectorTest */

struct BlockwiseChannelSelectorTestSuite : public vigra::test_suite {
    BlockwiseChannelSelectorTestSuite()
        : vigra::test_suite("BlockwiseChannelSelectorTestSuite")
    {
        add( testCase(&BlockwiseChannelSelectorTest::test) );
    }
};

int main(int argc, char ** argv) {
    BlockwiseChannelSelectorTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
