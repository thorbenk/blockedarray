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

