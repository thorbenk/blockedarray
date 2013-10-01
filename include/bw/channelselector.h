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
