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

#ifndef BW_CONNECTEDCOMPONENTS_H
#define BW_CONNECTEDCOMPONENTS_H

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cassert>

#include <boost/foreach.hpp>

#include <vigra/union_find.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/labelvolume.hxx>
#include <vigra/labelimage.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/multi_pointoperators.hxx>

#include <bw/roi.h>
#include <bw/blocking.h>
#include <bw/sourcehdf5.h>
#include <bw/sinkhdf5.h>
#include <bw/compressedarray.h>

namespace BW {

using vigra::detail::UnionFindArray;

template<int N, class T>
void blockMerge(
    UnionFindArray<int>& ufd,
    Roi<N> roi1,
    T offset1,
    const vigra::MultiArrayView<N,T>& block1,
    Roi<N> roi2,
    T offset2,
    const vigra::MultiArrayView<N,T>& block2
) {
    typedef vigra::MultiArrayView<N,T> ArrayView;

    Roi<N> intersection;
    if(!roi1.intersect(roi2, intersection)) { return; }

    ArrayView ov1 = block1.subarray(intersection.p - roi1.p, intersection.q -roi1.p);
    ArrayView ov2 = block2.subarray(intersection.p - roi2.p, intersection.q -roi2.p);
    if(ov1.shape() != ov2.shape()) { throw std::runtime_error("err"); }

    typename ArrayView::iterator it1 = ov1.begin();
    typename ArrayView::iterator it2 = ov2.begin();
    for(; it1 != ov1.end(); ++it1, ++it2) {
        int x = *it1+offset1;
        int y = *it2+offset2;
        //if(*it1 == 0) ufd.makeUnion(0,y);
        //if(*it2 == 0) ufd.makeUnion(x,0);
        ufd.makeUnion(x, y);
    }
}


/**
 * Compute connected components either in 2D or 3D using 4 or 6 neighborhood.
 */
template<int N, class T, class LabelType>
struct ConnectedComponentsComputer {
    static LabelType compute(const vigra::MultiArrayView<N,T>& in, vigra::MultiArrayView<N,LabelType>& out);
};

template<class T, class LabelType>
struct ConnectedComponentsComputer<2,T,LabelType> {
    static LabelType compute(const vigra::MultiArrayView<2,T>& in, vigra::MultiArrayView<2,LabelType>& out) {
        return vigra::labelImageWithBackground( in, out, false /*use 4-neighbors*/, 0 /*bg label*/);
    }
};

template<class T, class LabelType>
struct ConnectedComponentsComputer<3,T,LabelType> {
    static LabelType compute(const vigra::MultiArrayView<3,T>& in, vigra::MultiArrayView<3,LabelType>& out) {
         return vigra::labelVolumeWithBackground(in, out, vigra::NeighborCode3DSix(), 0);
    }
};

/**
 * Compute connected components block-wise (less limited to RAM)
 */
template<int N>
class ConnectedComponents {
    public:
    typedef typename Roi<N>::V V;

    typedef int LabelType;

    typedef CompressedArray<N,LabelType> Compressed;

    ConnectedComponents(Source<N,vigra::UInt8>* blockProvider, V blockShape)
        : blockProvider_(blockProvider)
        , blockShape_(blockShape)
    {
        using namespace vigra;
        using std::vector;
        using std::pair;

        //blocks are overlapping by 1 pixel in all directions
        V overlap; std::fill(overlap.begin(), overlap.end(), 1);

        //
        // run connected components on each block separately
        //

        Blocking<N> blocking( Roi<N>(V(), blockProvider->shape()), blockShape, overlap);
        blockRois_ = blocking.blocks();
        vector<LabelType> offsets(blockRois_.size()+1);
        ccBlocks_.resize(blockRois_.size());

        std::cout << "* run connected components on each of " << blockRois_.size() << " blocks separately" << std::endl;

        size_t sizeBytes = 0;
        size_t sizeBytesUncompressed = 0;

        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << " (#components: " << offsets[i] << ", curr compressed size: " << sizeBytes/(1024.0*1024.0) << " MB)                  \r" << std::flush;
            Roi<N>& block = blockRois_[i].second;
            //std::cout << "  - CC on block " << block << std::endl;

            //int maxLabel = vigra::labelVolumeWithBackground(
            //    vigra::srcMultiArrayRange( labels_.subarray(block.p, block.q) ),
            //    vigra::destMultiArray( ccBlocks[i] ), vigra::NeighborCode3DSix(), 0);

            MultiArray<N, vigra::UInt8> inBlock(block.q-block.p);
            blockProvider_->readBlock(block, inBlock);

            MultiArray<N, LabelType> cc(block.q-block.p);

            LabelType maxLabel = ConnectedComponentsComputer<N, vigra::UInt8, LabelType>::compute(inBlock, cc);
            //LabelType maxLabel = vigra::labelImageWithBackground(
            //    inBlock,
            //    cc,
            //    false /*use 4-neighbors*/,
            //    0 /*bg label*/);

            //ccBlocks_[i] = cc;

            ccBlocks_[i] = Compressed(cc);
            ccBlocks_[i].compress();

            sizeBytes += ccBlocks_[i].currentSizeBytes();
            sizeBytesUncompressed += ccBlocks_[i].uncompressedSizeBytes();

            offsets[i+1] = offsets[i] + maxLabel + 1;
        }
        std::cout << std::endl;
        std::cout << "  " << sizeBytes/(1024.0*1024.0) << " MB (vs. " << sizeBytesUncompressed/(1024.0*1024.0) << "MB uncompressed)" << std::endl;

        LabelType maxOffset = offsets[offsets.size()-1];

        std::cout << "  initialize union find datastructure with maxOffset = " << maxOffset << std::endl;
        UnionFindArray<LabelType> ufd(maxOffset);

        //
        // merge adjacent blocks
        //
        std::cout << "* merge adjacent blocks" << std::endl;

        //FIXME: use adjacency relation of blocks
        std::vector< std::pair<size_t, size_t> > adjBlocks;
        for(size_t i=0; i<blockRois_.size(); ++i) {
            for(int j=i+1; j<blockRois_.size(); ++j) {
                V bc1 = blockRois_[i].first;
                V bc2 = blockRois_[j].first;

                if(std::abs(vigra::squaredNorm(bc1-bc2)) == 1) {
                    adjBlocks.push_back(std::make_pair(i,j));
                }
            }
        }

        for(vector<pair<size_t, size_t> >::iterator it=adjBlocks.begin(); it!=adjBlocks.end(); ++it) {
            std::cout << "  pair " << std::distance(adjBlocks.begin(), it)+1 << "/" << adjBlocks.size() << "        \r" << std::flush;
            size_t i = it->first;
            size_t j = it->second;

            Roi<N>& block1 = blockRois_[i].second;
            Roi<N>& block2 = blockRois_[j].second;

            //cc1 = ccBlocks_[i];
            //cc2 = ccBlocks_[j];
            MultiArray<N,LabelType> cc1(block1.q-block1.p);
            MultiArray<N,LabelType> cc2(block2.q-block2.p);
            ccBlocks_[i].readArray(cc1);
            ccBlocks_[j].readArray(cc2);

            blockMerge<N, LabelType>(ufd,
                block1, offsets[i], cc1,
                block2, offsets[j], cc2);
        }
        std::cout << std::endl;

        LabelType maxLabel = ufd.makeContiguous();

        std::cout << "* relabel" << std::endl;
        sizeBytes = 0;
        sizeBytesUncompressed = 0;
        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << "        \r" << std::flush;
            Roi<N>& roi = blockRois_[i].second;

            //MultiArray<N,LabelType> cc = ccBlocks_[i];
            MultiArray<N,LabelType> cc(roi.q-roi.p); ccBlocks_[i].readArray(cc);

            LabelType offset = offsets[i];

            for(size_t j=0; j<cc.size(); ++j) { cc[j] = ufd[ cc[j]+offset ]; }

            //ccBlocks_[i] = cc;
            ccBlocks_[i] = Compressed(cc);
            ccBlocks_[i].compress();
            sizeBytes += ccBlocks_[i].currentSizeBytes();
            sizeBytesUncompressed += ccBlocks_[i].uncompressedSizeBytes();
        }
        std::cout << std::endl;
        std::cout << "  " << sizeBytes/(1024.0*1024.0) << " MB (vs. " << sizeBytesUncompressed/(1024.0*1024.0) << "MB uncompressed)" << std::endl;

    }

    void writeResult(const std::string& hdf5file, const std::string& hdf5group, int compression = 1) {
        using namespace vigra;

        vigra_precondition(compression >= 1 && compression <= 9, "compression must be >= 1 and <= 9");

        std::cout << "* write " << hdf5file << "/" << hdf5group << std::endl;
        
        HDF5File out(hdf5file, HDF5File::Open);
        out.createDataset<N, LabelType>(hdf5group, blockProvider_->shape(), 0, blockShape_, compression);
        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << "        \r" << std::flush;
            const Roi<N>& roi = blockRois_[i].second;

            //cc = ccBlocks_[i];
            MultiArray<N,LabelType> cc(roi.q-roi.p);
            ccBlocks_[i].readArray(cc);

            out.writeBlock(hdf5group, roi.p, cc);

        }
        std::cout << std::endl;
        out.close();
    }
    
    void writeToSink(Sink<N,LabelType>* sink)
    {
        std::cout << "* writing to sink" << std::endl;
        
        sink->setShape(blockProvider_->shape());
        sink->setBlockShape(blockShape_);
        
        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << "        \r" << std::flush;
            const Roi<N>& roi = blockRois_[i].second;

            vigra::MultiArray<N,LabelType> cc(roi.q-roi.p);
            ccBlocks_[i].readArray(cc);
            
            sink->writeBlock(roi, cc);
        }
        std::cout << std::endl;
    }

    private:
    Source<N,vigra::UInt8>* blockProvider_;
    V blockShape_;

    std::vector< std::pair<V, Roi<N> > > blockRois_;
    //std::vector< vigra::MultiArray<N,LabelType> > ccBlocks_;
    std::vector< Compressed > ccBlocks_;
};

} /* namespace BW */

#endif /* BW_CONNECTEDCOMPONENTS_H */
