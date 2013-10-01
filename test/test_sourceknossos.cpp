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
#include <iomanip>

#include <bw/sourceknossos.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct SourceKnossosTest {
void test() {
    using namespace vigra;
    typedef SourceKnossos<3, float>::V V;

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
