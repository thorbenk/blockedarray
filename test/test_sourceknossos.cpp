#include <iostream>
#include <iomanip>

#include <bw/sourceknossos.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>

using namespace BW;

struct SourceKnossosTest {
void test() {
    using namespace vigra;
    typedef typename SourceKnossos<3, float>::V V;
    
#if 0 /*FIXME*/
    V p(0,1000,0);
    V q(3841,1100,5120);
    SourceKnossos<3, unsigned char> sk("/path/to/knossos/folder");
    vigra::MultiArray<3, unsigned char> block(q-p); 

    sk.readBlock(Roi<3>(p, q), block);
   
    for(size_t i = 0; i<block.shape(1); i+=10) {
        vigra::MultiArray<2, unsigned char> slice = block.bind<1>(i);
        std::stringstream fname; fname << "/tmp/test" << std::setw(4) << std::setfill('0') << i << ".png";
        vigra::exportImage(vigra::srcImageRange(slice), vigra::ImageExportInfo(fname.str().c_str()));
    }
#endif

}
}; /* struct SourceKnossosTest */

struct SourceKnossosTestSuite : public vigra::test_suite {
    SourceKnossosTestSuite()
        : vigra::test_suite("SourceKnossosTestSuite")
    {
        add( testCase(&SourceKnossosTest::test) );
    }
};

int main(int argc, char ** argv) {
    SourceKnossosTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
