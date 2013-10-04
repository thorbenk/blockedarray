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

#ifndef BW_ARRAY_H
#define BW_ARRAY_H

#include <map>
#include <cassert>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include <vigra/timing.hxx>

#include "compressedarray.h"
#include <bw/roi.h>

template<int Dim, class Type>
class ArrayTest;

namespace BW {

template<int N, typename T>
class Array {
    public:
    typedef CompressedArray<N,T> BLOCK;
    typedef typename vigra::MultiArrayView<N,T>::difference_type V;
    typedef Roi<N> ROI;
    typedef typename std::vector<V> BlockList;
    typedef vigra::MultiArrayView<N,T> view_type;
    typedef boost::shared_ptr<BLOCK> BlockPtr;
    typedef std::map<V, BlockPtr> BlocksMap;
    typedef std::map<V, std::pair<T, T> > BlockMinMax;
    typedef std::pair<std::vector<V>, std::vector<T> > VoxelValues;
    typedef std::map<V, VoxelValues> BlockVoxelValues;

    //give unittest access
    friend class ArrayTest<N, T>;

    friend class RwIterator;
    /**
     * A helper iterator for read/write access to a region of interest
     */
    class RwIterator {
        public:
        RwIterator(const Array<N,T>& array, V p, V q);
        void next();
        bool hasMore() const;

        //given the current block (blockCoord), read the data from the input
        //array using read roi
        ROI read; //FIXME: find a better name here
        //given the current block (blockCoord), write the data into the block
        //using the withinBlock roi
        ROI withinBlock;

        V blockCoord;

        private:
        void compute();
        const Array<N,T>& array_;
        const ROI r;
        typename BlockList::const_iterator it_;
        BlockList blockList_;
        V blockP_;
    };

    /**
     * construct a new Array with given 'blockShape'
     *
     * post condition: numBlocks() == 0
     */
    Array(typename vigra::MultiArrayShape<N>::type blockShape);

    /**
     * construct a new Array with given 'blockShape' and initialize with data 'a'
     */
    Array(typename vigra::MultiArrayShape<N>::type blockShape, const vigra::MultiArrayView<N, T>& a);

    Array() {}

    typename vigra::MultiArrayShape<N>::type blockShape()
    {
    	return blockShape_;
    }

    static Array<N,T> readHDF5(hid_t group, const char* name);

    void writeHDF5(hid_t group, const char* name) const;

    /**
     * If coordinate lists management is enabled, a separate
     * sparse list of non-zero coordinates and their associated
     * values is kept for each stored block.
     *
     * Enabling coordinate lists implies 'delete empty blocks'.
     */
    void setManageCoordinateLists(bool manageCoordinateLists);

    /**
     * whether to check, on every write operation, whether a block has
     * become empty (contains) only zeros, and to delete such a block
     */
    void setDeleteEmptyBlocks(bool deleteEmpty);

    /**
     * If compression is enabled, all current blocks are compressed.
     * Furthermore, each newly added block is set to be compressed
     * by default.
     */
    void setCompressionEnabled(bool enableCompression);

    /**
     * Track the minimum and maximum of each block,
     * and enable the reporting of a global min/max for
     * the whole dataset
     */
    void setMinMaxTrackingEnabled(bool enableMinMaxTracking);

    /**
     * If min-max tracking is enabled, returns the min/max bounds of all currently
     * stored data.
     *
     * If min-max tracking is enabled (@see setMinMaxTrackingEnabled),
     * computes in O(#blocks) the minimum and maximum value of all currently stored
     * data.
     * If min-max tracking is disabled, returns an invalid min/max range.
     */
    std::pair<T, T> minMax() const;

    /**
     * returns the average compression ratio of all blocks currently in use
     */
    double averageCompressionRatio() const;

    /**
     *  returns the total number of blocks currently in use
     */
    size_t numBlocks() const { return blocks_.size(); }

    /**
     * returns the total size of all currently allocated blocks in bytes
     */
    size_t sizeBytes() const;

    /**
     * read array 'a' into 'out' from region of interest [p, q)
     *
     * If any of the needed blocks does not exist, 'out' will have
     * zeros at the corresponding locations.
     */
    void readSubarray(V p, V q,
                      vigra::MultiArrayView<N, T>& out) const;

    T operator[](V p) const;

    void write(V p, T value);

    /**
     * write array 'a' into the region of interest [p, q)
     *
     * If a block is _completely_ overwritten, its state is set to NOT DIRTY.
     * Otherwise (if a block is only partially overwritten), it dirty state remains UNCHANGED.
     */
    void writeSubarray(V p, V q,
                       const vigra::MultiArrayView<N, T>& a);

    /**
     * Like writeSubarray, but only overwrites if the pixel value in 'a'
     * is not zero. Input pixel with a value of 'writeAsZero' are written
     * as zero.
     */
    void writeSubarrayNonzero(V p, V q,
                              const vigra::MultiArrayView<N, T>& a,
                              T writeAsZero);

    /**
     * deletes all blocks which contain the region of interest [p,q)
     *
     * TODO: Is this the behaviour we want?
     *       Even blocks partially intersecting [p,q) are deleted!
     */
    void deleteSubarray(V p, V q);

    void applyRelabeling(const vigra::MultiArrayView<1, T>& relabeling);

    /**
     * set all blocks that intersect the ROI [p,q) to be flagged as 'dirty'
     */
    void setDirty(V p, V q, bool dirty);

    /**
     * Whether the roi [p,q) is dirty.
     * This function will not only consider dirtyness on a block level,
     * but also the dirtyness that is tracked on a per-slice level.
     */
    bool isDirty(V p, V q) const;

    /**
     * get a list of all blocks intersecting ROI [p,q) that are currently stored
     */
    BlockList blocks(V p, V q) const;

    /**
     * get a list of all blocks within the ROI [p,q) that are marked as dirty
     *
     * Returns: list of block coordinates which are dirty
     */
    BlockList dirtyBlocks(V p, V q) const;

    /**
     * compute the block bounds [p,q) given a block coordinate 'c'
     */
    void blockBounds(V c, V&p, V& q) const;

    VoxelValues nonzero() const;

    std::vector<V> enumerateBlocksInRange(V p, V q) const;

private:

    //delete block and all data associated with it
    // (including sparse coordinate lists, min/max information etc.)
    void deleteBlock(V blockCoord);

    VoxelValues blockNonzero(const vigra::MultiArrayView<N,T>& block) const;

    BlockPtr addBlock(V c, vigra::MultiArrayView<N, T> const & a);

    V blockGivenCoordinateP(V p) const;

    V blockGivenCoordinateQ(V q) const;

    bool allzero(const vigra::MultiArrayView<N,T>& block) const;

    std::pair<T, T> minMax(const vigra::MultiArrayView<N,T>& block) const;

    // members

    typename vigra::MultiArrayShape<N>::type blockShape_;
    BlocksMap blocks_;

    //for temporary storage of a block, to avoid repeated allocations
    mutable vigra::MultiArray<N,T> tmpBlock_;

    //whether to check, on every write operation, whether a block has
    //become empty (contains) only zeros, and to delete such a block
    bool deleteEmptyBlocks_;

    bool enableCompression_;

    bool minMaxTracking_;

    BlockMinMax blockMinMax_;

    bool manageCoordinateLists_;

    BlockVoxelValues blockVoxelValues_;
};

//==========================================================================//
// Constructors, Destructor                                                 //
//==========================================================================//

template<int N, typename T>
Array<N,T>::Array(typename vigra::MultiArrayShape<N>::type blockShape)
    : blockShape_(blockShape)
    , tmpBlock_(blockShape)
    , deleteEmptyBlocks_(false)
    , enableCompression_(false)
    , minMaxTracking_(false)
    , manageCoordinateLists_(false)
{
}

template<int N, typename T>
Array<N,T>::Array(
    typename vigra::MultiArrayShape<N>::type blockShape,
    const vigra::MultiArrayView<N, T>& a
)
    : blockShape_(blockShape)
    , tmpBlock_(blockShape)
    , deleteEmptyBlocks_(false)
    , enableCompression_(false)
    , minMaxTracking_(false)
    , manageCoordinateLists_(false)
{
    writeSubarray(V(), a.shape(), a);
}

//==========================================================================//
// operator[]                                                               //
//==========================================================================//

template<int N, typename T>
T Array<N,T>::operator[](V p) const {
    V blockCoord = blockGivenCoordinateP(p);

    typename BlocksMap::const_iterator it = blocks_.find(blockCoord);
    if(it==blocks_.end()) { return T(); }
    it->second->readArray(tmpBlock_);
    V pBlock;
    for(size_t i=0; i<N; ++i) { pBlock[i] = p[i] % blockShape_[i]; }
    return tmpBlock_[pBlock];
}

//==========================================================================//
// option setter                                                            //
//==========================================================================//

template<int N, typename T>
void Array<N,T>::setDeleteEmptyBlocks(bool deleteEmpty) {
    deleteEmptyBlocks_ = deleteEmpty;
}

template<int N, typename T>
void Array<N,T>::setManageCoordinateLists(bool manageCoordinateLists) {
    manageCoordinateLists_ = manageCoordinateLists;
    setDeleteEmptyBlocks(manageCoordinateLists);

    blockVoxelValues_.clear();
    if(manageCoordinateLists) {
        BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
            b.second->readArray(tmpBlock_);
            blockVoxelValues_[b.first] = blockNonzero(tmpBlock_);
        }
    }
}

template<int N, typename T>
void Array<N,T>::setCompressionEnabled(bool enableCompression) {
    enableCompression_ = enableCompression;

    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        if(enableCompression_) b.second->compress();
        else b.second->uncompress();
    }
}

template<int N, typename T>
void Array<N,T>::setMinMaxTrackingEnabled(bool enableMinMaxTracking) {
    minMaxTracking_ = enableMinMaxTracking;

    blockMinMax_.clear();
    if(minMaxTracking_) {
        BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
            b.second->readArray(tmpBlock_);
            blockMinMax_[b.first] = minMax(tmpBlock_);
        }
    }
}

//==========================================================================//
// option getter                                                            //
//==========================================================================//

template<int N, typename T>
std::pair<T, T> Array<N,T>::minMax() const {
    T m = std::numeric_limits<T>::max();
    T M = std::numeric_limits<T>::min();
    BOOST_FOREACH(const typename BlockMinMax::value_type& b, blockMinMax_) {
        if(b.second.first < m) m = b.second.first;
        if(b.second.second > M) M = b.second.second;
    }
    return std::make_pair(m,M);
}

template<int N, typename T>
typename Array<N,T>::VoxelValues
Array<N,T>::nonzero(
) const {
    VoxelValues ret;
    std::vector<V>& coords = ret.first;
    std::vector<T>& vals = ret.second;
    BOOST_FOREACH(const typename BlockVoxelValues::value_type& b, blockVoxelValues_) {
        const V& blockCoord = b.first;
        V p, q;
        blockBounds(blockCoord, p,q);
        const VoxelValues blockVV = b.second;
        for(size_t i=0; i<blockVV.first.size(); ++i) {
            coords.push_back( blockVV.first[i]+p );
            vals.push_back( blockVV.second[i] );
        }
    }
    return ret;
}

//==========================================================================//
// other getters                                                            //
//==========================================================================//

template<int N, typename T>
double Array<N,T>::averageCompressionRatio() const {
    double avg = 0.0;
    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        avg += b.second->compressionRatio();
    }
    return avg / blocks_.size();
}

template<int N, typename T>
size_t Array<N,T>::sizeBytes() const {
    size_t bytes = 0;
    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        bytes += b.second->currentSizeBytes();
    }
    return bytes;
}

//==========================================================================//
// read & write data                                                        //
//==========================================================================//

template<int N, typename T>
void Array<N,T>::readSubarray(
    V p, V q, vigra::MultiArrayView<N, T>& out
) const {
    using vigra::MultiArrayView;

    //make sure to initialize the array with zeros
    //if a block does not exist, we assume missing values of zero
    std::fill(out.begin(), out.end(), 0);

    vigra_precondition(out.shape()==q-p,"shape differ");

    for(RwIterator wIt(*this,p,q); wIt.hasMore(); wIt.next()) {
        typename BlocksMap::const_iterator it = blocks_.find(wIt.blockCoord);
        if(it==blocks_.end()) {
            //this block does not exist. //do nothing
            continue;
        }
        MultiArrayView<N,T> outView = out.subarray(wIt.read.p, wIt.read.q);
        it->second->readSubarray(&tmpBlock_, wIt.withinBlock.p, wIt.withinBlock.q, outView);
    }
}


template<int N, typename T>
void Array<N,T>::write(V p, T value) {
    V pBlock;
    for(size_t i=0; i<N; ++i) { pBlock[i] = p[i] % blockShape_[i]; }
    V blockCoord = blockGivenCoordinateP(p);
    typename BlocksMap::const_iterator it = blocks_.find(blockCoord);
    if(it==blocks_.end()) {
        std::fill(tmpBlock_.begin(), tmpBlock_.end(), 0);
        addBlock(blockCoord, tmpBlock_);
        it = blocks_.find(blockCoord);
    }
    it->second->readArray(tmpBlock_);
    tmpBlock_[pBlock] = value;

    if(deleteEmptyBlocks_ || minMaxTracking_ || manageCoordinateLists_) {
        bool blockDeleted = false;
        if(deleteEmptyBlocks_ && allzero(tmpBlock_)) {
            blockDeleted = true;
            deleteBlock(blockCoord);
        }
        if(!blockDeleted && minMaxTracking_) {
            blockMinMax_[it->first] = minMax(tmpBlock_);
        }
        if(!blockDeleted && manageCoordinateLists_) {
            blockVoxelValues_[it->first] = blockNonzero(tmpBlock_);
        }
    }
    it->second->writeArray(V(), tmpBlock_.shape(), tmpBlock_);
}

template<int N, typename T>
void Array<N,T>::writeSubarray(
    V p, V q, const vigra::MultiArrayView<N, T>& a
) {
    for(RwIterator wIt(*this,p,q); wIt.hasMore(); wIt.next()) {
        typename BlocksMap::iterator it = blocks_.find(wIt.blockCoord);

        //block does not exist, create it first
        if(it == blocks_.end()) {
            V block_p = wIt.withinBlock.p ;
            V block_q = wIt.withinBlock.q ;
            
            // Fast path: If subarray overlaps this block entirely,
            // copy it directly to new block to avoid a copy
            if (block_p == V() && block_q - block_p == blockShape_) {
                const view_type toWrite = a.subarray(wIt.read.p, wIt.read.q);
                BlockPtr ptr = addBlock(wIt.blockCoord, toWrite);
                ptr->setDirty(false);
            } else {
                // The array we were given doesn't span the entire block.
                // Add a full empty block, then copy from the subarray.
                vigra::MultiArray<N,T> emptyBlock(blockShape_);
                addBlock(wIt.blockCoord, emptyBlock);
                it = blocks_.find(wIt.blockCoord);
                const view_type toWrite = a.subarray(wIt.read.p, wIt.read.q);
                it->second->writeArray(wIt.withinBlock.p, wIt.withinBlock.q, toWrite);
            }
        }
        else {
            //write data to block
            const view_type toWrite = a.subarray(wIt.read.p, wIt.read.q);
            it->second->writeArray(wIt.withinBlock.p, wIt.withinBlock.q, toWrite);
        }

        //re-compute, if necessary, information from the _whole_ blocks's
        //data
        if(deleteEmptyBlocks_ || minMaxTracking_ || manageCoordinateLists_) {
            it->second->readArray(tmpBlock_);
            bool blockDeleted = false;
            if(deleteEmptyBlocks_ && allzero(tmpBlock_)) {
                blockDeleted = true;
                deleteBlock(wIt.blockCoord);
            }
            if(!blockDeleted && minMaxTracking_) {
                blockMinMax_[it->first] = minMax(tmpBlock_);
            }
            if(!blockDeleted && manageCoordinateLists_) {
                blockVoxelValues_[it->first] = blockNonzero(tmpBlock_);
            }
        }
    }
}

template<int N, typename T>
void Array<N,T>::writeSubarrayNonzero(
    V p, V q,
    const vigra::MultiArrayView<N, T>& a,
    T writeAsZero
) {
    for(RwIterator wIt(*this,p,q); wIt.hasMore(); wIt.next()) {
        typename BlocksMap::iterator it = blocks_.find(wIt.blockCoord);

        //make tmpBlock_ hold the current block data
        if(it == blocks_.end()) {
            std::fill(tmpBlock_.begin(), tmpBlock_.end(), 0);
            addBlock(wIt.blockCoord, tmpBlock_);
            it = blocks_.find(wIt.blockCoord);
        }
        else {
            it->second->readArray(tmpBlock_);
        }

        const view_type inData  = a.subarray(wIt.read.p, wIt.read.q);
        view_type curData = tmpBlock_.subarray(wIt.withinBlock.p, wIt.withinBlock.q);

        for(size_t i=0; i<inData.size(); ++i) {
            const T in = inData[i];
            if(in == 0) { continue; }
            if(in == writeAsZero) {
                curData[i] = 0;
            }
            else {
                curData[i] = in;
            }
        }

        it->second->writeArray(wIt.withinBlock.p, wIt.withinBlock.q, curData);

        //re-compute, if necessary, information from the _whole_ blocks's
        //data
        if(deleteEmptyBlocks_ || minMaxTracking_ || manageCoordinateLists_) {
            bool blockDeleted = false;
            if(deleteEmptyBlocks_ && allzero(tmpBlock_)) {
                blockDeleted = true;
                deleteBlock(wIt.blockCoord);
            }
            if(!blockDeleted && minMaxTracking_) {
                blockMinMax_[it->first] = minMax(tmpBlock_);
            }
            if(!blockDeleted && manageCoordinateLists_) {
                blockVoxelValues_[it->first] = blockNonzero(tmpBlock_);
            }
        }
    }
}

//==========================================================================//
// delete data                                                              //
//==========================================================================//

template<int N, typename T>
void Array<N,T>::deleteSubarray(V p, V q) {
    const BlockList bb = enumerateBlocksInRange(p, q);
    BOOST_FOREACH(V blockCoor, bb) {
        deleteBlock(blockCoor);
    }
}

//==========================================================================//
// data transformations                                                     //
//==========================================================================//

template<int N, typename T>
void Array<N,T>::applyRelabeling(
    const vigra::MultiArrayView<1, T>& relabeling
) {
    BOOST_FOREACH(typename BlocksMap::value_type& b, blocks_) {
        b.second->readArray(tmpBlock_);
        for(size_t i=0; i<tmpBlock_.size(); ++i) {
            tmpBlock_[i] = relabeling[static_cast<size_t>(tmpBlock_[i]) % relabeling.size()];
        }
        if(deleteEmptyBlocks_ || minMaxTracking_ || manageCoordinateLists_) {
            bool blockDeleted = false;
            if(deleteEmptyBlocks_ && allzero(tmpBlock_)) {
                blockDeleted = true;
                deleteBlock(b.first);
            }
            if(!blockDeleted && minMaxTracking_) {
                blockMinMax_[b.first] = minMax(tmpBlock_);
            }
            if(!blockDeleted && manageCoordinateLists_) {
                blockVoxelValues_[b.first] = blockNonzero(tmpBlock_);
            }
        }
        b.second->writeArray(V(), tmpBlock_.shape(), tmpBlock_);
    }
}

//==========================================================================//
// dirtyness                                                                //
//==========================================================================//

template<int N, typename T>
bool Array<N,T>::isDirty(V p, V q) const {
    for(RwIterator wIt(*this,p,q); wIt.hasMore(); wIt.next()) {
        typename BlocksMap::const_iterator it = blocks_.find(wIt.blockCoord);
        if(it==blocks_.end()) {
            return true; //FIXME: semantically correct?
        }
        if( it->second->isDirty(wIt.withinBlock.p, wIt.withinBlock.q) ) {
            return true;
        }
    }
    return false;
}

template<int N, typename T>
void Array<N,T>::setDirty(V p, V q, bool dirty) {
    for(RwIterator wIt(*this,p,q); wIt.hasMore(); wIt.next()) {
        typename BlocksMap::const_iterator it = blocks_.find(wIt.blockCoord);
        if(it==blocks_.end()) {
            continue;
        }
        it->second->setDirty(wIt.withinBlock.p, wIt.withinBlock.q, dirty);
    }
}

template<int N, typename T>
typename Array<N,T>::BlockList Array<N,T>::dirtyBlocks(V p, V q) const {
    BlockList dB;
    const BlockList bb = blocks(p, q);
    BOOST_FOREACH(V blockCoor, bb) {
        typename BlocksMap::const_iterator it = blocks_.find(blockCoor);
        if(it == blocks_.end()) {
            dB.push_back(blockCoor);
        }
        else if(it->second->isDirty()) {
            dB.push_back(blockCoor);
        }
    }
    return dB;
}

//==========================================================================//
// blocks                                                                   //
//==========================================================================//

template<int N, typename T>
typename Array<N,T>::BlockList Array<N,T>::blocks(V p, V q) const {
    BlockList bL;
    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        bL.push_back(b.first);
    }
    return bL;
}


template<int N, typename T>
void Array<N,T>::blockBounds(
    V c, V&p, V& q) const {
    for(int i=0; i<N; ++i) {
        p[i] = blockShape_[i]*c[i];
        q[i] = blockShape_[i]*(c[i]+1);
        #ifdef DEBUG_CHECKS
        CHECK_OP(q[i],>,p[i]," ");
        #endif
    }
}

//==== IMPLEMENTATION (RwIterator) =====//

template<int N, typename T>
Array<N,T>::RwIterator::RwIterator(
    const Array<N,T>& array,
    typename Array<N,T>::V p,
    typename Array<N,T>::V q
)
    : array_(array)
    , r(p,q)
{
    blockList_ = array.enumerateBlocksInRange(p, q); //bb
    blockP_ = array.blockGivenCoordinateP(p); //blockP
    it_ = blockList_.begin();
    compute();
}

template<int N, typename T>
void Array<N,T>::RwIterator::compute() {
    blockCoord = *it_;
    V bp, bq;
    array_.blockBounds(*it_, bp, bq);

    withinBlock = ROI();
    read = ROI();

    //compute within-block coordinates
    for(int k=0; k<N; ++k) {
        if(bp[k] <= r.p[k] && r.p[k] < bq[k]) {
            withinBlock.p[k] = r.p[k] % array_.blockShape_[k];
        }
        if(r.q[k] > bp[k] && r.q[k] < bq[k]) {
            withinBlock.q[k] = r.q[k] % array_.blockShape_[k];
        }
        else {
            withinBlock.q[k] = array_.blockShape_[k];
        }
    }
    //compute where to read 'w' from
    for(int k=0; k<N; ++k) {
        const int d = (*it_)[k] - blockP_[k];
        if(d >= 1) {
            read.p[k] += array_.blockShape_[k] - (r.p[k] % array_.blockShape_[k]);
        }
        if(d >= 2) {
            read.p[k] += (d-1)*array_.blockShape_[k];
        }
        read.q[k] = read.p[k]+(withinBlock.shape())[k];
    }
}

template<int N, typename T>
void Array<N,T>::RwIterator::next() {
    ++it_;
    if(hasMore()) compute();
}

template<int N, typename T>
bool Array<N,T>::RwIterator::hasMore() const {
    return it_ != blockList_.end();
}

//==== IMPLEMENTATION (private member functions) =====//

template<int N, typename T>
typename Array<N,T>::BlockPtr Array<N,T>::addBlock(
    V c,
    vigra::MultiArrayView<N, T> const & a
) {
    BlockPtr ca(new BLOCK(vigra::MultiArrayView<N, T, vigra::StridedArrayTag>(a)));
    ca->setDirty(true);
    blocks_[c] = ca; //TODO: use std::move here
    if(enableCompression_) {
        ca->compress();
    }
    return ca;
}

template<int N, typename T>
std::vector<typename Array<N,T>::V> Array<N,T>::enumerateBlocksInRange(
    V p,
    V q
) const {
    const V blockP = blockGivenCoordinateP(p);
    const V blockQ = blockGivenCoordinateQ(q);

    std::vector<V> ret;

    V x = blockP;
    int dim=N-1;
    ret.push_back(x);
    while(dim >= 0) {
        while(dim >= 0) {
            if(x[dim] >= blockQ[dim]-1) {
                x[dim] = blockP[dim];
                --dim;
                continue;
            } else {
                ++x[dim];
                ret.push_back(x);
                break;
            }
        }
        if(dim >= 0) {
            dim = N-1;
        }
    }
    return ret;
}


template<int N, typename T>
typename Array<N,T>::V
Array<N,T>::blockGivenCoordinateP(V p) const {
    V c;
    for(int i=0; i<N; ++i) { c[i] = p[i]/blockShape_[i]; }
    return c;
}

template<int N, typename T>
typename Array<N,T>::V
Array<N,T>::blockGivenCoordinateQ(V q) const {
    V c;
    for(int i=0; i<N; ++i) { c[i] = (q[i]-1)/blockShape_[i] + 1; }
    return c;
}

template<int N, typename T>
bool Array<N,T>::allzero(const vigra::MultiArrayView<N,T>& block) const {
    for(size_t i=0; i<block.size(); ++i) {
        if(block[i] != 0) return false;
    }
    return true;
}

template<int N, typename T>
std::pair<T, T> Array<N,T>::minMax(
    const vigra::MultiArrayView<N,T>& block
) const {
    vigra::FindMinMax<T> minmax;
    vigra::inspectSequence(block.begin(), block.end(), minmax);
    return std::make_pair(minmax.min, minmax.max);
}

template<int N, typename T>
typename Array<N,T>::VoxelValues
Array<N,T>::blockNonzero(const vigra::MultiArrayView<N,T>& block) const {
    VoxelValues ret;
    std::vector<V>& coords = ret.first;
    std::vector<T>& vals = ret.second;

    for(size_t i=0; i<block.size(); ++i) {
        const T& v = block[i];
        if(v == 0) { continue; }
        coords.push_back( block.scanOrderIndexToCoordinate(i) );
        vals.push_back(v);
    }
    return ret;
}

template<int N, typename T>
void Array<N,T>::deleteBlock(V blockCoord) {
    typename BlocksMap::iterator it = blocks_.find(blockCoord);
    if(it == blocks_.end()) return;

    blocks_.erase(it);
    typename BlockMinMax::iterator it2 = blockMinMax_.find(blockCoord);
    if(it2 != blockMinMax_.end()) {
        blockMinMax_.erase(it2);
    }
    typename BlockVoxelValues::iterator it3 = blockVoxelValues_.find(blockCoord);
    if(it3 != blockVoxelValues_.end()) {
        blockVoxelValues_.erase(it3);
    }
}

template<int N, typename T>
Array<N,T> Array<N,T>::readHDF5(hid_t group, const char* name) {
    hsize_t adims[2];

    Array<N,T> a;

    hid_t baGroup  = H5Gopen(group, name, H5P_DEFAULT);

    //blockShape_ attribute
    {
        hid_t attr = H5Aopen(baGroup, "sh", H5P_DEFAULT);
        uint32_t sh[N];
        H5Aread(attr, H5T_NATIVE_UINT32 /*memtype*/, sh);
        H5Aclose(attr);
        std::copy(sh, sh+N, a.blockShape_.begin());
    }

    //blocks
    if(H5Lexists(baGroup, "blocks", H5P_DEFAULT)) {
        hid_t blocksDset = H5Dopen(baGroup, "blocks", H5P_DEFAULT);
        hid_t filetype   = H5Dget_type(blocksDset);
        hid_t space      = H5Dget_space(blocksDset);

        H5Sget_simple_extent_dims(space, adims, NULL);
        uint32_t* coords = new uint32_t[adims[0]*adims[1]];
        H5Dread(blocksDset, H5T_STD_U32LE /*memtype*/, H5S_ALL, H5S_ALL, H5P_DEFAULT, coords);

        for(size_t i=0; i<adims[0]; ++i) {
            V coord;
            for(size_t j=0; j<N; ++j) {
                coord[j] = coords[N*i+j];
            }
            std::stringstream g; g << i << "d";
            BlockPtr ca = BlockPtr(new CompressedArray<N,T>());

            *ca = CompressedArray<N,T>::readHDF5(baGroup, g.str().c_str());
            a.blocks_[coord] = ca;
        }

        delete[] coords;

        H5Sclose(space);
        H5Tclose(filetype);
        H5Dclose(blocksDset);
    }

    a.deleteEmptyBlocks_     = H5A<bool>::read(baGroup, "deb");
    a.enableCompression_     = H5A<bool>::read(baGroup, "ec");
    a.minMaxTracking_        = H5A<bool>::read(baGroup, "mmt");
    a.manageCoordinateLists_ = H5A<bool>::read(baGroup, "mcl");

    if(a.minMaxTracking_ && H5Aexists(baGroup, "minMax")) {
        hid_t attr       = H5Aopen(baGroup, "minMax", H5P_DEFAULT);
        hid_t filetype   = H5Aget_type(attr);
        hid_t space      = H5Aget_space(attr);

        H5Sget_simple_extent_dims(space, adims, NULL);
        T* mM = new T[adims[0]*adims[1]];
        H5Aread(attr, H5Type<T>::get_NATIVE(), mM);

        assert(adims[0] == a.blocks_.size());
        assert(adims[1] == 2);

        size_t i = 0;
        BOOST_FOREACH(const typename BlocksMap::value_type& b, a.blocks_) {
            std::pair<T,T> minMax;
            minMax.first = mM[2*i+0];
            minMax.second = mM[2*i+1];
            a.blockMinMax_[b.first] = minMax;
            ++i;
        }

        delete[] mM;

        H5Sclose(space);
        H5Tclose(filetype);
        H5Aclose(attr);
    }

    if(a.manageCoordinateLists_) {
        size_t i = 0;
        BOOST_FOREACH(const typename BlocksMap::value_type& b, a.blocks_) {
            a.blockVoxelValues_[b.first] = std::make_pair(std::vector<V>(), std::vector<T>());
            std::vector<V>& idx = a.blockVoxelValues_[b.first].first;
            std::vector<T>& val = a.blockVoxelValues_[b.first].second;

            std::stringstream idxG; idxG << i << "s-idx";
            if(H5Lexists(baGroup, idxG.str().c_str(), H5P_DEFAULT)) {
                hid_t idxDset     = H5Dopen(baGroup, idxG.str().c_str(), H5P_DEFAULT);
                hid_t idxFiletype = H5Dget_type(idxDset);
                hid_t idxSpace    = H5Dget_space(idxDset);
                hsize_t idxDims[2];

                H5Sget_simple_extent_dims(idxSpace, idxDims, NULL);

                size_t sz = idxDims[0]*idxDims[1];
                if(sz > 0) {
                uint32_t* idx_array = new uint32_t[sz];
                H5Dread(idxDset, H5T_STD_U32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, idx_array);
                idx.resize(idxDims[0]);
                for(size_t j=0; j<idxDims[0]; ++j) {
                    for(size_t k=0; k<N; ++k) {
                    idx[j][k] = idx_array[N*j+k];
                    }
                }
                delete[] idx_array;
                }

                H5Sclose(idxSpace);
                H5Tclose(idxFiletype);
                H5Dclose(idxDset);

                //val
                std::stringstream valG; valG << i << "s-val";
                hid_t valDset     = H5Dopen(baGroup, valG.str().c_str(), H5P_DEFAULT);
                hid_t valFiletype = H5Dget_type(valDset);
                hid_t valSpace    = H5Dget_space(valDset);
                hsize_t valDim;
                H5Sget_simple_extent_dims(valSpace, &valDim, NULL);

                if(valDim > 0) {
                val.resize(valDim);
                H5Dread(valDset, H5Type<T>::get_STD_LE(), H5S_ALL, H5S_ALL, H5P_DEFAULT, &val[0]);
                }

                H5Sclose(valSpace);
                H5Tclose(valFiletype);
                H5Dclose(valDset);
            }

            ++i;
        }
    }

    H5Gclose(baGroup);

    return a;
}

template<int N, typename T>
void Array<N,T>::writeHDF5(hid_t group, const char* name) const {
    hid_t gr = H5Gcreate(group, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    uint32_t* coords = new uint32_t[blocks_.size()*N];

    size_t i = 0;
    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        std::stringstream g; g << i << "d";
        b.second->writeHDF5(gr, g.str().c_str());
        for(size_t j=0; j<N; ++j) {
            coords[N*i+j] = b.first[j];
        }
        ++i;
    }

    //shape_;
    {
        hsize_t n = N;
        uint32_t* sh = new uint32_t[N];
        std::copy(blockShape_.begin(), blockShape_.end(), sh);

        hid_t space    = H5Screate_simple(1, &n, NULL);
        hid_t attr     = H5Acreate(gr, "sh", H5T_STD_U32LE, space, H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, H5T_NATIVE_UINT32, sh);
        H5Aclose(attr);
        H5Sclose(space);

        delete[] sh;
    }

    //write mapping block coordinate -> block dataset
    if(blocks_.size() > 0) {
        hsize_t x[2] = {blocks_.size(), N};

        hid_t space     = H5Screate_simple(2, x, NULL);
        hid_t dataset   = H5Dcreate(gr, "blocks", H5T_STD_U32LE, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        H5Dwrite(dataset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, coords);

        H5Dclose(dataset);
        H5Sclose(space);
    }

    H5A<bool>::write(gr, "deb", deleteEmptyBlocks_);
    H5A<bool>::write(gr, "ec",  enableCompression_);
    H5A<bool>::write(gr, "mmt", minMaxTracking_);
    H5A<bool>::write(gr, "mcl", manageCoordinateLists_);

    if(blockMinMax_.size() > 0) {
        hsize_t x[2] = {blockMinMax_.size(), 2};

        hid_t space  = H5Screate_simple(2, x, NULL);
        hid_t attr   = H5Acreate(gr, "minMax", H5Type<T>::get_STD_LE(), space, H5P_DEFAULT, H5P_DEFAULT);

        T* mM = new T[2*blockMinMax_.size()];
        size_t i = 0;
        assert(blocks_.size() == blockMinMax_.size());
        BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
            assert(blockMinMax_.find(b.first) != blockMinMax_.end());
            typename BlockMinMax::mapped_type x = blockMinMax_.find(b.first)->second;
            mM[2*i+0] = x.first;
            mM[2*i+1] = x.second;
            ++i;
        }

        H5Awrite(attr, H5Type<T>::get_NATIVE(), mM);

        H5Aclose(attr);
        H5Sclose(space);

        delete[] mM;
    }

    if(manageCoordinateLists_ && blockVoxelValues_.size() > 0) {
        size_t i = 0;
        BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
            assert(blockVoxelValues_.find(b.first) != blockVoxelValues_.end());
            typename BlockVoxelValues::mapped_type x = blockVoxelValues_.find(b.first)->second;
            const std::vector<V>& idx = x.first;
            const std::vector<T>& val = x.second;

            //for each block (e.g. block 42), we create a group called 42s-idx
            //  (where s stands for sparse)
            if(idx.size() > 0) {
                std::stringstream gIdx; gIdx << i << "s-idx";

                const size_t rows = idx.size();

                hsize_t shape[2] = {rows, N};
                hid_t space   = H5Screate_simple(2, shape, NULL);
                hid_t dataset = H5Dcreate(gr, gIdx.str().c_str(), H5T_STD_U32LE, space,
                                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                if(rows*N > 0) {
                    uint32_t* idx_array = new uint32_t[rows*N];
                    for(size_t i=0; i<idx.size(); ++i) {
                        for(size_t j=0; j<N; ++j) {
                            idx_array[N*i+j] = idx[i][j];
                        }
                    }
                    H5Dwrite(dataset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, idx_array);
                    delete[] idx_array;
                }
                H5Dclose(dataset);
                H5Sclose(space);
            }

            //also, write the voxel values for each row
            if(val.size() > 0) {
                std::stringstream gVal; gVal << i << "s-val";
                hsize_t rows = val.size();
                hid_t space   = H5Screate_simple(1, &rows, NULL);
                hid_t dataset = H5Dcreate(gr, gVal.str().c_str(), H5Type<T>::get_STD_LE(), space,
                                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                if(val.size() > 0) {
                    H5Dwrite(dataset, H5Type<T>::get_NATIVE(), H5S_ALL, H5S_ALL, H5P_DEFAULT, &val[0]);
                }
                H5Dclose(dataset);
                H5Sclose(space);
            }

            ++i;
        }
    }

    H5Gclose(gr);

    delete[] coords;
}

} /* namespace BW */

#endif /* BW_ARRAY_H */
