#ifndef BLOCKWISETHRESHOLDING_H
#define BLOCKWISETHRESHOLDING_H

#include "blockedsource.h"
#include "blockedsink.h"
#include "blocking.h"

/**
 * Blockwise thresholding (not limited by RAM)
 * 
 * Reads data with dimension N, pixel type T in a block-wise fashion from a HDF5File,
 * performs thresholding, and writes to a chunked and compressed output file.
 */
template<int N, class T>
class BlockwiseThresholding {
    public:
    
    typedef typename Roi<N>::V V;
    BlockwiseThresholding(BlockedSource<N,T>* source, V blockShape)
        : blockShape_(blockShape)
        , shape_(source->shape())
        , source_(source)
    {
        vigra_precondition(shape_.size() == N, "dataset shape is wrong");
        
        Roi<N> roi(V(), shape_);
        Blocking<N> bb(roi, blockShape, V());
        std::cout << "* BlockwiseThresholding with " << bb.numBlocks() << " blocks" << std::endl;
        blocking_ = bb;
    }
    
    /**
     * Applies thresholding 'threshold'  to the image. If a pixel value is greater than 'threshold',
     * it is assigned the 'ifLower' value, otherwise the 'ifHigher' value. 
     */
    void run(T threshold, vigra::UInt8 ifLower, vigra::UInt8 ifHigher, BlockedSink<N,vigra::UInt8>* sink) {
        using namespace vigra;
        
        sink->setShape(shape_);
        
        int blockNum = 0;
        typename Blocking<N>::Pair p;
        BOOST_FOREACH(p, blocking_.blocks()) {
            std::cout << "  block " << blockNum+1 << "/" << blocking_.numBlocks() << "        \r" << std::flush;
            Roi<N> roi = p.second;
            
            MultiArray<N, T> inBlock(roi.shape());
            source_->readBlock(roi, inBlock); 
            
            MultiArray<N, UInt8> outBlock(inBlock.shape());
            T* a     = inBlock.data();
            UInt8* b = outBlock.data();
            for(size_t k=0; k<inBlock.size(); ++k) {
                *b = (*a > threshold) ? ifHigher : ifLower;
                ++a; ++b;
            }
            sink->writeBlock(roi, outBlock);
            ++blockNum;
        }
    }
    
    private:
    V shape_;
    V blockShape_;
    Blocking<N> blocking_;
    BlockedSource<N,T>* source_;
};

#endif /* BLOCKWISETHRESHOLDING_H */
