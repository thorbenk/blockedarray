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

#ifndef BW_THRESHOLDING_H
#define BW_THRESHOLDING_H

#include <bw/source.h>
#include <bw/sink.h>
#include <bw/blocking.h>

namespace BW {

/**
 *  thresholding (not limited by RAM)
 *
 * Reads data with dimension N, pixel type T in a block-wise fashion from a HDF5File,
 * performs thresholding, and writes to a chunked and compressed output file.
 */
template<int N, class T>
class Thresholding {
    public:

    typedef typename Roi<N>::V V;
    Thresholding(Source<N,T>* source, V blockShape)
        : blockShape_(blockShape)
        , shape_(source->shape())
        , source_(source)
    {
        vigra_precondition(shape_.size() == N, "dataset shape is wrong");

        Roi<N> roi(V(), shape_);
        Blocking<N> bb(roi, blockShape, V());
        std::cout << "* Thresholding with " << bb.numBlocks() << " blocks" << std::endl;
        blocking_ = bb;
    }

    /**
     * Applies thresholding 'threshold'  to the image. If a pixel value is greater than 'threshold',
     * it is assigned the 'ifLower' value, otherwise the 'ifHigher' value.
     */
    void run(T threshold, vigra::UInt8 ifLower, vigra::UInt8 ifHigher, Sink<N,vigra::UInt8>* sink) {
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
    Source<N,T>* source_;
};

} /* namespace BW */

#endif /* BW_THRESHOLDING_H */
