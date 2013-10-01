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
#include <bw/channelselector.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct ChannelSelectorTest {
void test() {
    using namespace vigra;
    typedef ChannelSelector<4, float>::V V;

    for(int ch=0; ch<=1; ++ch) {
        std::cout << "* channel = " << ch << std::endl;

        MultiArray<4, float> data(vigra::TinyVector<int, 4>(10,20,30,2));
        FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
        {
            HDF5File f("test.h5", HDF5File::Open);
            f.write("test", data);
        }

        SourceHDF5<4, float> source("test.h5", "test");
        SinkHDF5<3, float> sink("channel.h5", "channel");
        sink.setBlockShape(V(10,10,10));

        ChannelSelector<4, float> cs(&source, V(10,10,10));

        cs.run(3, ch, &sink);

        HDF5File f("channel.h5", HDF5File::Open);
        MultiArray<3, float> r;
        f.readAndResize("channel", r);

        MultiArrayView<3, float> shouldResult = data.bind<3>(ch);

        shouldEqualSequence(r.begin(), r.end(), shouldResult.begin());
    } //loop through channels
}

void testRoi() {
    using namespace vigra;
    typedef ChannelSelector<4, float>::V V;
    typedef vigra::TinyVector<int, 4> V4;

    int ch = 0;

    MultiArray<4, float> data(vigra::TinyVector<int, 4>(10,20,30,2));
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
    {
        HDF5File f("test.h5", HDF5File::Open);
        f.write("test", data);
    }

    SourceHDF5<4, float> source("test.h5", "test");
    source.setRoi(Roi<4>(V4(1,3,5,0), V4(7,9,30,2)));

    SinkHDF5<3, float> sink("channel.h5", "channel");

    ChannelSelector<4, float> cs(&source, V(10,10,10));

    cs.run(3, ch, &sink);

    HDF5File f("channel.h5", HDF5File::Open);
    MultiArray<3, float> r;
    f.readAndResize("channel", r);

    MultiArrayView<3, float> shouldResult = data.bind<3>(ch).subarray(V(1,3,5), V(7,9,30));

    shouldEqualSequence(r.begin(), r.end(), shouldResult.begin());
}

}; /* struct ChannelSelectorTest */

struct ChannelSelectorTestSuite : public vigra::test_suite {
    ChannelSelectorTestSuite()
        : vigra::test_suite("ChannelSelectorTestSuite")
    {
        add( testCase(&ChannelSelectorTest::test) );
        add( testCase(&ChannelSelectorTest::testRoi) );
    }
};

int main(int argc, char ** argv) {
    ChannelSelectorTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
