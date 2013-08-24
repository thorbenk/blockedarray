#ifndef BW_CHANNELSELECTOR_H
#define BW_CHANNELSELECTOR_H

#include <bw/source.h>
#include <bw/sink.h>
#include <bw/blocking.h>

namespace BW {

/**
 *  channel selector.
 * 
 * FIXME: Assumes channel is first axis
 */
template<int N, class T>
class ChannelSelector {
    public:
    
    typedef typename Roi<N-1>::V V;
    
    ChannelSelector(Source<N,T>* source, V blockShape)
        : blockShape_(blockShape)
        , source_(source)
    {
    }
    
    void run(int dim, int channel, Sink<N-1, T>* sink) {
        using namespace vigra;
        using std::vector;
        
        //read input shape
        typename Roi<N>::V sh = source_->shape();
        
        std::cout << "* blockwise channel selector on dataset with input shape = " << sh << std::endl;
        vigra_precondition(sh.size() == N, "dataset shape is wrong");
       
        //compute output shape
        {
            int j=0;
            for(int i=0; i<N; ++i) {
                if(i == dim) continue;
                shape_[j] = sh[i]; ++j;
            }
        }
            
        std::cout << "  output shape: " << shape_;
        Roi<N-1> roi(V(), shape_);
        blocking_ = Blocking<N-1>(roi, blockShape_, V());
        std::cout << ", divided into " << blocking_.numBlocks() << " blocks of shape " << blockShape_ << std::endl;

        sink->setShape(shape_);
        
        int blockNum = 0;
        vector<typename Blocking<N-1>::Pair> blocks = blocking_.blocks();
        for(int i=0; i<blocks.size(); ++i) {
            const Roi<N-1>& roi = blocks[i].second;
            
            std::cout << "  block " << i+1 << "/" << blocks.size() << "        \r" << std::flush;
            
            Roi<N> newRoi = roi.insertAxisBefore(dim, channel, channel+1);
            MultiArray<N, T> inBlock(newRoi.q - newRoi.p);
            source_->readBlock(newRoi, inBlock);
          
            MultiArrayView<N-1, T> outBlock = inBlock.bindAt(dim, 0 /*newRoi has a singleton dim here*/);
            sink->writeBlock(roi, outBlock);
            ++blockNum;
        }
        std::cout << std::endl;
    }
    
    private:
    V blockShape_;
    Source<N,T>* source_;
    V shape_;
    Blocking<N-1> blocking_;
};

} /* namespace BW */

#endif /* BW_SELECTOR_H */
