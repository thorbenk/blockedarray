#ifndef BW_ARRAY_H
#define BW_ARRAY_H

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include <vigra/timing.hxx>

#include "compressedarray.h"

template<int Dim, class Type>
class ArrayTest;
    
namespace BW {

template<int N, typename T>
class Array {
    public:
    typedef CompressedArray<N,T> BLOCK;
    typedef vigra::TinyVector<unsigned int, N> BlockCoord;
    typedef typename vigra::MultiArrayView<N,T>::difference_type difference_type;
    typedef typename std::vector<BlockCoord> BlockList;
    typedef vigra::MultiArrayView<N,T> view_type;
    typedef boost::shared_ptr<BLOCK> BlockPtr; 
    typedef std::map<BlockCoord, BlockPtr> BlocksMap;
    typedef std::map<BlockCoord, std::pair<T, T> > BlockMinMax;
   
    //give unittest access
    friend class ArrayTest<N, T>;
    
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
    size_t numBlocks() const;
    
    /**
     * returns the total size of all currently allocated blocks in bytes
     */
    size_t sizeBytes() const;

    /**
     * write array 'a' into the region of interest [p, q)
     * 
     * If a block is _completely_ overwritten, its state is set to NOT DIRTY.
     * Otherwise (if a block is only partially overwritten), it dirty state remains UNCHANGED.
     */
    void writeSubarray(difference_type p, difference_type q,
                       const vigra::MultiArrayView<N, T>& a);
   
    /**
     * deletes all blocks which contain the region of interest [p,q)
     */
    void deleteSubarray(difference_type p, difference_type q);
   
    /**
     * set all blocks that intersect the ROI [p,q) to be flagged as 'dirty'
     */
    void setDirty(difference_type p, difference_type q, bool dirty);
    
    /**
     * get a list of all blocks within the ROI [p,q) that are marked as dirty
     * 
     * Returns: list of block coordinates which are dirty
     */
    BlockList dirtyBlocks(difference_type p, difference_type q) const ;

    /**
     * read array 'a' into 'out' from region of interest [p, q)
     * 
     * If any of the needed blocks does not exist, 'out' will have
     * zeros at the corresponding locations.
     */
    void readSubarray(difference_type p, difference_type q,
                      vigra::MultiArrayView<N, T>& out) const;
    
    /**
     * compute the block bounds [p,q) given a block coordinate 'c'
     */
    void blockBounds(BlockCoord c, difference_type&p, difference_type& q) const;
    
    private:
        
    BlockPtr addBlock(BlockCoord c, vigra::MultiArrayView<N, T>& a);

    std::vector<BlockCoord> blocks(difference_type p, difference_type q) const;

    BlockCoord blockGivenCoordinateP(difference_type p) const;

    BlockCoord blockGivenCoordinateQ(difference_type q) const;

    bool allzero(const BLOCK& block) const;

    std::pair<T, T> minMax(const BLOCK& block) const;

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
};

//==== IMPLEMENTATION ====//

template<int N, typename T>
Array<N,T>::Array(typename vigra::MultiArrayShape<N>::type blockShape)
    : blockShape_(blockShape)
    , tmpBlock_(blockShape)
    , deleteEmptyBlocks_(false)
    , enableCompression_(false)
    , minMaxTracking_(false)
{
}

template<int N, typename T>
Array<N,T>::Array(typename vigra::MultiArrayShape<N>::type blockShape, const vigra::MultiArrayView<N, T>& a)
    : blockShape_(blockShape)
    , tmpBlock_(blockShape)
    , deleteEmptyBlocks_(false)
    , enableCompression_(false)
    , minMaxTracking_(false)
{
    writeSubarray(difference_type(), a.shape(), a);
}

template<int N, typename T>
void Array<N,T>::setDeleteEmptyBlocks(bool deleteEmpty) {
    deleteEmptyBlocks_ = deleteEmpty;
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
            blockMinMax_[b.first] = minMax(*(b.second.get()));
        }
    }
}

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
double Array<N,T>::averageCompressionRatio() const {
    double avg = 0.0;
    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        avg += b.second->compressionRatio();
    }
    return avg / blocks_.size();
}

template<int N, typename T>
size_t Array<N,T>::numBlocks() const {
    return blocks_.size();
}

template<int N, typename T>
size_t Array<N,T>::sizeBytes() const {
    size_t bytes = 0;
    BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
        bytes += b.second->currentSizeBytes();
    }
    return bytes;
}

template<int N, typename T>
void Array<N,T>::writeSubarray(
    difference_type p,
    difference_type q,
    const vigra::MultiArrayView<N, T>& a
) {
    const BlockList bb = blocks(p, q);
    const BlockCoord blockP = blockGivenCoordinateP(p);
    
    BOOST_FOREACH(BlockCoord blockCoor, bb) {
        difference_type bp, bq;
        blockBounds(blockCoor, bp, bq);
        
        //compute within-block coordinates
        difference_type withinBlock_p;
        difference_type withinBlock_q;
        
        for(int k=0; k<N; ++k) {
            if(bp[k] <= p[k] && p[k] < bq[k]) {
                withinBlock_p[k] = p[k]%blockShape_[k];
            }
            if(q[k] > bp[k] && q[k] < bq[k]) {
                withinBlock_q[k] = q[k]%blockShape_[k];
            }
            else {
                withinBlock_q[k] = blockShape_[k];
            }
        }
        
        //compute where to read 'w' from
        difference_type read_p, read_q;
        for(int k=0; k<N; ++k) {
            const int d = blockCoor[k] - blockP[k];
            if(d >= 1) {
                read_p[k] += blockShape_[k] - (p[k] % blockShape_[k]);
            }
            if(d >= 2) {
                read_p[k] += (d-1)*blockShape_[k];
            }
            read_q[k] = read_p[k]+(withinBlock_q-withinBlock_p)[k];
        }
        
        #ifdef DEBUG_PRINTS
        std::cout << "blockCoor=" << blockCoor << std::endl;
        std::cout << "bp = " << bp << std::endl;
        std::cout << "bq = " << bq << std::endl;
        std::cout << "withinBlock_p = " << withinBlock_p << std::endl;
        std::cout << "withinBlock_q = " << withinBlock_q << std::endl;
        std::cout << "read_p = " << read_p << std::endl;
        std::cout << "read_q = " << read_q << std::endl;
        #endif
        
        typename BlocksMap::iterator it = blocks_.find(blockCoor);
        
        if(it == blocks_.end()) {
            vigra::MultiArray<N,T> emptyBlock(blockShape_);
            addBlock(blockCoor, emptyBlock);
            it = blocks_.find(blockCoor);
        }
        const view_type toWrite = a.subarray(read_p, read_q);
        it->second->writeArray(withinBlock_p, withinBlock_q, toWrite);

        if(deleteEmptyBlocks_ && allzero(*(it->second.get()))) {
            blocks_.erase(it);
            typename BlockMinMax::iterator it2 = blockMinMax_.find(blockCoor);
            if(it2 != blockMinMax_.end()) {
                blockMinMax_.erase(it2);
            }
        }
        else if(minMaxTracking_) {
            blockMinMax_[it->first] = minMax(*(it->second.get()));
        }
    }
}

template<int N, typename T>
void Array<N,T>::deleteSubarray(difference_type p, difference_type q) {
    const BlockList bb = blocks(p, q);
    BOOST_FOREACH(BlockCoord blockCoor, bb) {
        typename BlocksMap::iterator it = blocks_.find(blockCoor);
        if(it != blocks_.end()) {
            blocks_.erase(it);
        }
        
        typename BlockMinMax::iterator it2 = blockMinMax_.find(blockCoor);
        if(it2 != blockMinMax_.end()) {
            blockMinMax_.erase(it2);
        }
    }
}

template<int N, typename T>
void Array<N,T>::setDirty(difference_type p, difference_type q, bool dirty) {
    const BlockList bb = blocks(p, q);
    BOOST_FOREACH(BlockCoord blockCoor, bb) {
        typename BlocksMap::iterator it = blocks_.find(blockCoor);
        if(it == blocks_.end()) { continue; }
        it->second->setDirty(dirty);
    }
}

template<int N, typename T>
typename Array<N,T>::BlockList Array<N,T>::dirtyBlocks(difference_type p, difference_type q) const {
    BlockList dB;
    const BlockList bb = blocks(p, q);
    BOOST_FOREACH(BlockCoord blockCoor, bb) {
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

template<int N, typename T>
void Array<N,T>::readSubarray(
    difference_type p,
    difference_type q,
    vigra::MultiArrayView<N, T>& out
) const {
    using vigra::MultiArrayView;

    //make sure to initialize the array with zeros
    //if a block does not exist, we assume missing values of zero
    std::fill(out.begin(), out.end(), 0);
    
    vigra_precondition(out.shape()==q-p,"shape differ");
    
    //find affected blocks
    const BlockList bb = blocks(p, q);

    #ifdef DEBUG_PRINTS
    std::cout << "readSubarray(" << p << ", " << q << ")" << std::endl;
    std::cout << "  there are " << bb.size() << " blocks" << std::endl;
    #endif

    const BlockCoord blockP = blockGivenCoordinateP(p);

    double timeWritearray = 0.0;
    double timeReadarray  = 0.0;

    BOOST_FOREACH(BlockCoord blockCoor, bb) {
        difference_type bp, bq;
        blockBounds(blockCoor, bp, bq);

        #ifdef DEBUG_CHECKS
        for(int k=0; k<N; ++k) {
            CHECK_OP(p[k],<=,bq[k]," ");
            CHECK_OP(q[k],>=,bp[k]," ");
        }
        #endif
        #ifdef DEBUG_PRINTS
        std::cout << "  block = " << blockCoor << " with bounds " << bp << " -- " << bq << std::endl;
        #endif

        //compute within-block coordinates
        difference_type withinBlock_p;
        difference_type withinBlock_q;
        for(int k=0; k<N; ++k) {
            if(bp[k] <= p[k] && p[k] < bq[k]) {
                withinBlock_p[k] = p[k]%blockShape_[k];
            }
            if(q[k] > bp[k] && q[k] < bq[k]) {
                withinBlock_q[k] = q[k]%blockShape_[k];
            }
            else {
                withinBlock_q[k] = blockShape_[k];
            }
        }

        #ifdef DEBUG_PRINTS
        std::cout << "    withinBlock_p = " << withinBlock_p << std::endl;
        std::cout << "    withinBlock_q = " << withinBlock_q << std::endl;
        #endif
        #ifdef DEBUG_CHECKS
        for(int k=0; k<N; ++k) {
            CHECK_OP(withinBlock_p[k],>=,0," ");
            CHECK_OP(withinBlock_p[k],<=,blockShape_[k]," ");
            CHECK_OP(withinBlock_q[k],>,0," ");
            CHECK_OP(withinBlock_q[k],<=,blockShape_[k]," ");
            CHECK_OP(withinBlock_q[k],>,withinBlock_p[k]," ");
        }
        #endif

        //read in the current block and extract the appropriate subarray            
        typename BlocksMap::const_iterator it = blocks_.find(blockCoor);
        
        if(it==blocks_.end()) {
            //this block does not exist.
            //do nothing
            continue;
        }

        USETICTOC
        
        TIC
        it->second->readArray(tmpBlock_);
        timeReadarray += TOCN;
        
        MultiArrayView<N,T> v(tmpBlock_);

        #ifdef DEBUG_PRINTS
        std::cout << "    v.shape = " << v.shape() << std::endl;
        #endif
        #ifdef DEBUG_CHECKS
        for(int k=0; k<N; ++k) {
            CHECK_OP(v.shape(k),>=,0," ");
            CHECK_OP(v.shape(k),>=,withinBlock_q[k]," ");
        }
        #endif

        const view_type w = v.subarray(withinBlock_p, withinBlock_q);

        #ifdef DEBUG_PRINTS
        std::cout << "    read w with shape = " << w.shape() << std::endl;
        for(int k=0; k<N; ++k) {
            std::cout << "    p[" << k << "] = " << p[k] << std::endl;
            std::cout << "    p[" << k << "] % blockShape_[k] = " << p[k] % blockShape_[k] << std::endl;
        }
        #endif

        //compute where to write 'w' to
        difference_type write_p, write_q;
        for(int k=0; k<N; ++k) {
            const int d = blockCoor[k] - blockP[k];
            if(d >= 1) {
                write_p[k] += blockShape_[k] - (p[k] % blockShape_[k]);
            }
            if(d >= 2) {
                write_p[k] += (d-1)*blockShape_[k];
            }
            write_q[k] = write_p[k]+w.shape(k);
        }

        #ifdef DEBUG_PRINTS
        std::cout << "    write_p = " << write_p << std::endl;
        std::cout << "    write_q = " << write_q << std::endl;
        #endif

        #ifdef DEBUG_CHECKS 
        for(int k=0; k<N; ++k) {
            const int d = blockCoor[k] - blockP[k];
            CHECK_OP(write_p[k],>=,0," ");
            CHECK_OP(write_q[k],>,write_p[k]," ");
            CHECK_OP(write_q[k],<=,out.shape(k)," ");
        }
        #endif


        //std::cout << "writing to " << write_p << ", " << write_q << std::flush;
        //boost::timer::cpu_timer t; 
        TIC
        out.subarray(write_p, write_q) = w;
        timeWritearray += TOCN;
        //double e = t.elapsed().user / sec;
        //std::cout << " ... " << std::setprecision(20) << e << " sec." << std::endl;

        //std::cout << "one loop iteration took" << std::setprecision(20) << startLoop.elapsed().user/sec << " sec." << std::endl;

    }
    //std::cout << "c++ readSubarray took " << std::setprecision(20) << startMethod.elapsed().user/sec << " sec." << std::endl;
    //std::cout << "  writeSubarray took " << timeWritearray << " msec." << std::endl;
    //std::cout << "  readArray() took " << timeReadarray << " msec." << std::endl;
}

template<int N, typename T>
void Array<N,T>::blockBounds(
    BlockCoord c, difference_type&p, difference_type& q) const {
    for(int i=0; i<N; ++i) {
        p[i] = blockShape_[i]*c[i];
        q[i] = blockShape_[i]*(c[i]+1);
        #ifdef DEBUG_CHECKS
        CHECK_OP(q[i],>,p[i]," "); 
        #endif
    }
}

//==== IMPLEMENTATION (private member functions) =====//

template<int N, typename T>
typename Array<N,T>::BlockPtr Array<N,T>::addBlock(
    BlockCoord c,
    vigra::MultiArrayView<N, T>& a
) {
    vigra::MultiArray<N,T> block(blockShape_);
    block.subarray(difference_type(), a.shape()) = a;

    BlockPtr ca(new BLOCK(block));
    ca->setDirty(true);
    blocks_[c] = ca; //TODO: use std::move here
    if(enableCompression_) {
        ca->compress();
    }
    return ca;
}

template<int N, typename T>
std::vector<typename Array<N,T>::BlockCoord> Array<N,T>::blocks(
    difference_type p,
    difference_type q
) const {
    const BlockCoord blockP = blockGivenCoordinateP(p);
    const BlockCoord blockQ = blockGivenCoordinateQ(q);

    std::vector<BlockCoord> ret;

    BlockCoord x = blockP;
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
typename Array<N,T>::BlockCoord
Array<N,T>::blockGivenCoordinateP(difference_type p) const {
    BlockCoord c;
    for(int i=0; i<N; ++i) { c[i] = p[i]/blockShape_[i]; }
    return c;
}

template<int N, typename T>
typename Array<N,T>::BlockCoord
Array<N,T>::blockGivenCoordinateQ(difference_type q) const {
    BlockCoord c;
    for(int i=0; i<N; ++i) { c[i] = (q[i]-1)/blockShape_[i] + 1; }
    return c;
}

template<int N, typename T>
bool Array<N,T>::allzero(const BLOCK& block) const {
    block.readArray(tmpBlock_);
    for(size_t i=0; i<tmpBlock_.size(); ++i) {
        if(tmpBlock_[i] != 0) return false;
    }
    return true;
}

template<int N, typename T>
std::pair<T, T> Array<N,T>::minMax(const BLOCK& block) const {
    block.readArray(tmpBlock_);
    vigra::FindMinMax<T> minmax;
    vigra::inspectSequence(tmpBlock_.begin(), tmpBlock_.end(), minmax);
    return std::make_pair(minmax.min, minmax.max);
}

} /* namespace BW */

#endif /* BW_ARRAY_H */
