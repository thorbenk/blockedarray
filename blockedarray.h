#ifndef BLOCKEDARRAY_H
#define BLOCKEDARRAY_H

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "compressedarray.h"

template<int N, class T, class BLOCK = CompressedArray<N,T> >
class BlockedArray {
    public:
    typedef vigra::TinyVector<unsigned int, N> BlockCoord;
    typedef typename vigra::MultiArrayView<N,T>::difference_type difference_type;
    typedef typename std::vector<BlockCoord> BlockList;
    typedef vigra::MultiArrayView<N,T> view_type;
    typedef boost::shared_ptr<BLOCK> BlockPtr; 
    typedef std::map<BlockCoord, BlockPtr> BlocksMap;
    
    /**
     * construct a new BlockedArray with given 'blockShape'
     * 
     * post condition: numBlocks() == 0
     */
    BlockedArray(typename vigra::MultiArrayShape<N>::type blockShape)
        : blockShape_(blockShape)
    {
    }

    /**
     * construct a new BlockedArray with given 'blockShape' and initialize with data 'a'
     */
    BlockedArray(typename vigra::MultiArrayShape<N>::type blockShape, const vigra::MultiArrayView<N, T>& a)
        : blockShape_(blockShape)
    {
        writeSubarray(difference_type(), a.shape(), a);
    }
    
    /**
     * returns the average compression ratio of all blocks currently in use
     */
    double averageCompressionRatio() {
        double avg = 0.0;
        BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
            avg += b.second->compressionRatio();
        }
        return avg / blocks_.size();
    }
   
    /**
     *  returns the total number of blocks currently in use
     */
    size_t numBlocks() const {
        return blocks_.size();
    }
    
    /**
     * returns the total size of all currently allocated blocks in bytes
     */
    size_t sizeBytes() const {
        size_t bytes = 0;
        BOOST_FOREACH(const typename BlocksMap::value_type& b, blocks_) {
            bytes += b.second->currentSizeBytes();
        }
        return bytes;
    }

    /**
     * write array 'a' into the region of interest [p, q)
     * 
     * If a block is _completely_ overwritten, its state is set to NOT DIRTY.
     * Otherwise (if a block is only partially overwritten), it dirty state remains UNCHANGED.
     */
    void writeSubarray(difference_type p, difference_type q, const vigra::MultiArrayView<N, T>& a) {
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
            
            typename BlocksMap::iterator it = blocks_.find(blockCoor);
            
            if(it == blocks_.end()) {
                vigra::MultiArray<N,T> emptyBlock(blockShape_);
                addBlock(blockCoor, emptyBlock);
                it = blocks_.find(blockCoor);
            }
            const view_type toWrite = a.subarray(read_p, read_q);
            it->second->writeArray(withinBlock_p, withinBlock_q, toWrite);
            
            if(withinBlock_p == difference_type() && withinBlock_q == blockShape_) {
                it->second->setDirty(false);
            }
            
        }
    }
   
    /**
     * deletes all blocks which contain the region of interest [p,q)
     */
    void deleteSubarray(difference_type p, difference_type q) {
        const BlockList bb = blocks(p, q);
        BOOST_FOREACH(BlockCoord blockCoor, bb) {
            typename BlocksMap::iterator it = blocks_.find(blockCoor);
            if(it == blocks_.end()) { continue; }
            blocks_.erase(it);
        }
    }
   
    /**
     * set all blocks that intersect the ROI [p,q) to be flagged as 'dirty'
     */
    void setDirty(difference_type p, difference_type q, bool dirty) {
        const BlockList bb = blocks(p, q);
        BOOST_FOREACH(BlockCoord blockCoor, bb) {
            typename BlocksMap::iterator it = blocks_.find(blockCoor);
            if(it == blocks_.end()) { continue; }
            it->second->setDirty(dirty);
        }
    }
    
    /**
     * get a list of all blocks within the ROI [p,q) that are marked as dirty
     * 
     * Returns: list of block coordinates which are dirty
     */
    BlockList dirtyBlocks(difference_type p, difference_type q) {
        BlockList dB;
        const BlockList bb = blocks(p, q);
        BOOST_FOREACH(BlockCoord blockCoor, bb) {
            typename BlocksMap::iterator it = blocks_.find(blockCoor);
            if(it == blocks_.end()) {
                dB.push_back(blockCoor);
            }
            else if(it->second->isDirty()) {
                dB.push_back(blockCoor);
            }
        }
        return dB;
    }

    /**
     * read array 'a' into 'out' from region of interest [p, q)
     * 
     * If any of the needed blocks does not exist, 'out' will have
     * zeros at the corresponding locations.
     */
    void readSubarray(difference_type p, difference_type q, vigra::MultiArrayView<N, T>& out) const {
        //make sure to initialize the array with zeros
        //if a block does not exist, we assume missing values of zero
        std::fill(out.begin(), out.end(), 0);
        
        using vigra::MultiArray;
        
        vigra_precondition(out.shape()==q-p,"shape differ");
        
        #ifdef DEBUG_PRINTS
        std::cout << "readSubarray(" << p << ", " << q << ")" << std::endl;
        #endif

        //find affected blocks
        const BlockList bb = blocks(p, q);

        const BlockCoord blockP = blockGivenCoordinateP(p);

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
            
            MultiArray<N,T> v = it->second->readArray();

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

            out.subarray(write_p, write_q) = w;

        }
    }
    
    /**
     * compute the block bounds [p,q) given a block coordinate 'c'
     */
    void blockBounds(BlockCoord c, difference_type&p, difference_type& q) const {
        for(int i=0; i<N; ++i) {
            p[i] = blockShape_[i]*c[i];
            q[i] = blockShape_[i]*(c[i]+1);
            #ifdef DEBUG_CHECKS
            CHECK_OP(q[i],>,p[i]," "); 
            #endif
        }
    }
    
    private:
        
    BlockPtr addBlock(BlockCoord c, vigra::MultiArrayView<N, T>& a) {
        vigra::MultiArray<N,T> block(blockShape_);
        block.subarray(difference_type(), a.shape()) = a;

        BlockPtr ca(new BLOCK(block));
        blocks_[c] = ca; //TODO: use std::move here
        return ca;
    }

    std::vector<BlockCoord> blocks(difference_type p, difference_type q) const {
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

    BlockCoord blockGivenCoordinateP(difference_type p) const {
        BlockCoord c;
        for(int i=0; i<N; ++i) { c[i] = p[i]/blockShape_[i]; }
        return c;
    }

    BlockCoord blockGivenCoordinateQ(difference_type q) const {
        BlockCoord c;
        for(int i=0; i<N; ++i) { c[i] = (q[i]-1)/blockShape_[i] + 1; }
        return c;
    }

    // members
    
    typename vigra::MultiArrayShape<N>::type blockShape_;
    BlocksMap blocks_;
};

#endif /* BLOCKEDARRAY_H */
