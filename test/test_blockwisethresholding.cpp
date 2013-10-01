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
