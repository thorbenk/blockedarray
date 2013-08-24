#include <iostream>

#include "blockwisechannelselector.h"
#include "blockwisethresholding.h"
#include "blockwisecc.h"

int main(int argc, char** argv) {
    using namespace vigra;
    
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
        HDF5BlockedSource<4, float> source(hdf5file, hdf5group);
        HDF5BlockedSink<3, float> sink("01_channel0.h5", "channel0");
        sink.setBlockShape(blockShape);
        BlockwiseChannelSelector<4, float> cs(&source, blockShape);
        cs.run(0, 0, &sink);
    }
    
    //threshold
    {
        HDF5BlockedSource<3, float> source("01_channel0.h5", "channel0");
        HDF5BlockedSink<3, vigra::UInt8> sink("02_thresh.h5", "thresh");
        sink.setBlockShape(blockShape);
        BlockwiseThresholding<3, float> bs(&source, blockShape);
        bs.run(0.5, 0, 1, &sink);
    }
    
    //connected components
    {
        HDF5BlockedSource<3, UInt8> source("02_thresh.h5", "thresh");
        BlockwiseConnectedComponents<3> bs(&source, blockShape);
        bs.writeResult("03_cc.h5", "cc", 1);
    }
    
}
    