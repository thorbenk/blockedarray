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

#ifdef _MSC_VER
#pragma warning (disable:4503)
#endif

#include <iostream>

#include <bw/sourcehdf5.h>
#include <bw/sinkhdf5.h>
#include <bw/regionfeatures.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>
#include <vigra/hdf5impex.hxx>

#include <bw/extern_templates.h>

using namespace BW;

struct RegionFeaturesTest {
void test() {
    using namespace vigra;
    typedef RegionFeatures<3, float, uint32_t>::V V;

    MultiArray<3, float> data(V(123,230,400), 1.0);
    FillRandom<float, float*>::fillRandom(data.data(), data.data()+data.size());
    {
        HDF5File f("test_data.h5", HDF5File::Open);
        f.write("data", data);
    }

    MultiArray<3, uint32_t> labels(data.shape());
    labels.subarray(V(20,30,40), V(30,40,50))    = 1;
    labels.subarray(V(30,50,60), V(35,80,100))   = 2;
    labels.subarray(V(99,99,99), V(100,100,100)) = 3;
    {
        HDF5File f("test_labels.h5", HDF5File::Open);
        f.write("labels", labels);
    }

    SourceHDF5<3, float> dataSource("test_data.h5", "data");
    SourceHDF5<3, uint32_t> labelsSource("test_labels.h5", "labels");

    RegionFeatures<3, float, uint32_t> bs(&dataSource, &labelsSource, V(75,75,75));

    bs.run("test_result.h5");
}
}; /* struct RegionFeaturesTest */

struct RegionFeaturesTestSuite : public vigra::test_suite {
    RegionFeaturesTestSuite()
        : vigra::test_suite("RegionFeaturesTestSuite")
    {
        add( testCase(&RegionFeaturesTest::test) );
    }
};

int main(int argc, char ** argv) {
    RegionFeaturesTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
