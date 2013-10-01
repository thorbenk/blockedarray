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

#include <bw/channelselector.h>
#include <bw/thresholding.h>
#include <bw/connectedcomponents.h>
#include <bw/regionfeatures.h>

#include <bw/extern_templates.h>

int main(int argc, char** argv) {
    using namespace vigra;
    using namespace BW;

    if(argc != 3) {
        std::cout << "usage: ./ccpipeline hdf5file hdf5group" << std::endl;
        return 0;
    }
    std::string hdf5file(argv[1]);
    std::string hdf5group(argv[2]);

    typedef TinyVector<int, 3> V;

    V blockShape(100,100,100);

    //extract channel 0
    {
        SourceHDF5<4, float> source(hdf5file, hdf5group);
        SinkHDF5<3, float> sink("01_channel0.h5", "channel0");
        sink.setBlockShape(blockShape);
        ChannelSelector<4, float> cs(&source, blockShape);
        cs.run(0, 0, &sink);
    }

    //threshold
    {
        SourceHDF5<3, float> source("01_channel0.h5", "channel0");
        SinkHDF5<3, vigra::UInt8> sink("02_thresh.h5", "thresh");
        sink.setBlockShape(blockShape);
        Thresholding<3, float> bs(&source, blockShape);
        bs.run(0.5, 0, 1, &sink);
    }

    //connected components
    {
        SourceHDF5<3, UInt8> source("02_thresh.h5", "thresh");
        ConnectedComponents<3> bs(&source, blockShape);
        bs.writeResult("03_cc.h5", "cc", 1);
    }

    //region features
    {
        SourceHDF5<3, float> sourceData("02_thresh.h5", "thresh");
        SourceHDF5<3, uint32_t> sourceLabels("02_thresh.h5", "thresh");
        RegionFeatures<3, float, uint32_t> rf(&sourceData, &sourceLabels, blockShape);
        vigra::MultiArray<2, float> out;
        rf.run("test_result.h5");
    }

}

