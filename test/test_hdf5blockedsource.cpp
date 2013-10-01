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

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct SourceHDF5Test {
void test() {
    using namespace vigra;
    typedef SourceHDF5<3, float>::V V;

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
