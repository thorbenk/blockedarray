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

#ifndef BW_RESAMPLEIMAGE_H
#define BW_RESAMPLEIMAGE_H

#include <bw/source.h>
#include <bw/sink.h>
#include <bw/blocking.h>

#include <vigra/multi_resize.hxx>
#include <vigra/timing.hxx>

namespace BW {
   
template<int N, class T>
void resample(
    vigra::MultiArrayView<N, T, vigra::StridedArrayTag>& in,
    vigra::MultiArrayView<N, T, vigra::StridedArrayTag>& out,
    double factor
) {
    typedef vigra::MultiArrayView<N, T> A;
    typedef typename A::difference_type V;
    V s;
    std::fill(s.begin(), s.end(), std::abs(1.0/factor));
    A v = in.stridearray(s);
    typename A::difference_type end = vigra::min(out.shape(), v.shape());
    out.subarray(V(), end).copy(v.subarray(V(), end));
}

/**
 *  resample an image (blockwise)
 */
template<int N, class T>
class ResampleImage {
    public:

    typedef typename Roi<N>::V V;
    ResampleImage(Source<N,T>* source, V blockShape)
        : blockShape_(blockShape)
        , shape_(source->shape())
        , source_(source)
    {
        vigra_precondition(shape_.size() == N, "dataset shape is wrong");

        Roi<N> roi(V(), shape_);
        Blocking<N> bb(roi, blockShape, V());
        std::cout << "* ResampleImage with " << bb.numBlocks() << " blocks of shape " << blockShape << std::endl;
        blocking_ = bb;
    }

    void run(double factor, Sink<N,T>* sink, V blockShape) {
        using namespace vigra;
        USETICTOC;
        
        double timeRead = 0.0;
        double timeResample = 0.0;
        double timeWrite = 0.0;

        V newShape = factor < 1.0 ? vigra::ceil(factor*source_->shape()) : vigra::floor(factor*source_->shape());
        
        sink->setShape(newShape);

        int blockNum = 0;
        typename Blocking<N>::Pair p;
        BOOST_FOREACH(p, blocking_.blocks()) {
            std::cout << "  block " << blockNum+1 << "/" << blocking_.numBlocks();
            if(blockNum != 0) {
                std::cout << " read: " << timeRead/((double)blockNum);
                std::cout << ", resample: " << timeResample/((double)blockNum);
                std::cout << ", write: " << timeWrite/((double)blockNum);
            }
            std::cout << "        \r" << std::flush;
            Roi<N> roi = p.second;

            TIC;
            MultiArray<N, T> inBlock(roi.shape());
            source_->readBlock(roi, inBlock);
            timeRead += TOCN;

            TIC;
            V outBlockShape = factor < 1.0 ? vigra::ceil(factor*inBlock.shape()) : vigra::floor(factor*inBlock.shape());
            MultiArray<N, T> outBlock(outBlockShape);
            //vigra::resizeMultiArraySplineInterpolation(inBlock, outBlock, vigra::BSpline<0, T>());
            resample<N,T>(inBlock, outBlock, factor);
            timeResample+= TOCN;
           
            TIC;
            Roi<N> outRoi(factor < 1.0 ? vigra::ceil(factor*roi.p) : vigra::floor(factor*roi.p), V());
            outRoi.q = outRoi.p + outBlock.shape();
            sink->writeBlock(outRoi, outBlock);
            timeWrite+= TOCN;
            
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

#endif /* BW_RESAMPLEIMAGE_H */
