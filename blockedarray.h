#ifndef BLOCKEDARRAY_H
#define BLOCKEDARRAY_H

#include "compressedarray.h"

template<int N, class T>
class BlockedArray {
    public:
    typedef vigra::TinyVector<unsigned int, N> BlockCoord;
    typedef typename vigra::MultiArrayView<N,T>::difference_type difference_type;

    BlockedArray(typename vigra::MultiArrayShape<N>::type blockShape, const vigra::MultiArrayView<N, T>& a)
        : blockShape_(blockShape)
        , shape_(a.shape())
    {
        auto bb = blocks(BlockCoord(), a.shape());

        for(auto x : bb) {
            std::cout << x << std::endl;

            difference_type p;
            difference_type q;
            for(int i=0; i<N; ++i) {
                p[i] = blockShape_[i]*x[i];
                q[i] = std::min( blockShape_[i]*(x[i]+1), a.shape(i) );
            }
            auto v = a.subarray(p, q);
            addBlock(x, v);
        }
    }

    void addBlock(BlockCoord c, vigra::MultiArrayView<N, T>& a) {
        vigra::MultiArray<N,T> block(blockShape_);
        block.subarray(difference_type(), a.shape()) = a;

        std::unique_ptr<CompressedArray<N, T> > ca(new CompressedArray<N, T>(block));
        std::cout << "  comp. ratio " << ca->compressionRatio() << std::endl;
        blocks_[c] = std::move(ca);
    }

    std::vector<BlockCoord> blocks(difference_type p, difference_type q) const {
        const auto blockP = blockGivenCoordinateP(p);
        const auto blockQ = blockGivenCoordinateQ(q);

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
        for(int i=0; i<N; ++i) { c[i] = std::min(p[i]/blockShape_[i], shape_[i]/blockShape_[i]); }
        return c;
    }

    BlockCoord blockGivenCoordinateQ(difference_type q) const {
        BlockCoord c;
        for(int i=0; i<N; ++i) { c[i] = std::min((q[i]-1)/blockShape_[i], shape_[i]/blockShape_[i]) + 1; }
        return c;
    }

    void blockBounds(BlockCoord c, difference_type&p, difference_type& q) const {
        for(int i=0; i<N; ++i) {
            p[i] = blockShape_[i]*c[i];
            q[i] = blockShape_[i]*(c[i]+1);
            #ifdef DEBUG_CHECKS
            CHECK_OP(q[i],>,p[i]," "); 
            #endif
        }
    }

    void writeSubarray(difference_type p, difference_type q, const vigra::MultiArrayView<N, T>& a) {
        const auto bb = blocks(p, q);
        const BlockCoord blockP = blockGivenCoordinateP(p);
        
        for(auto blockCoor : bb) {
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
            
            auto it = blocks_.find(blockCoor);
            const auto toWrite = a.subarray(read_p, read_q);
            
            it->second->writeArray(withinBlock_p, withinBlock_q, toWrite);
            
            //CHECK_OP((it->second->readArray().subarray(withinBlock_p, withinBlock_q)(0,0,0)),==,toWrite(0,0,0)," ");
            //CHECK_OP((it->second->readArray().subarray(withinBlock_p, withinBlock_q)(1,1,1)),==,toWrite(1,1,1)," ");
        }
    }

    vigra::MultiArray<N, T> readSubarray(difference_type p, difference_type q) const {
        #ifdef DEBUG_PRINTS
        std::cout << "readSubarray(" << p << ", " << q << ")" << std::endl;
        #endif

        //find affected blocks
        const auto bb = blocks(p, q);

        const BlockCoord blockP = blockGivenCoordinateP(p);

        vigra::MultiArray<N,T> a(q-p);

        for(auto blockCoor : bb) {
            
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
            auto it = blocks_.find(blockCoor);
            #if DEBUG_CHECKS
            if(it==blocks_.end()) { throw std::runtime_error("badder"); }
            #endif
            auto v = it->second->readArray();

            #ifdef DEBUG_PRINTS
            std::cout << "    v.shape = " << v.shape() << std::endl;
            #endif
            #ifdef DEBUG_CHECKS
            for(int k=0; k<N; ++k) {
                CHECK_OP(v.shape(k),>=,0," ");
                CHECK_OP(v.shape(k),>=,withinBlock_q[k]," ");
            }
            #endif

            const auto w = v.subarray(withinBlock_p, withinBlock_q);

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
                CHECK_OP(write_q[k],<=,a.shape(k)," ");
            }
            #endif

            a.subarray(write_p, write_q) = w;

        }

        return a;

    }

    private:
    typename vigra::MultiArrayShape<N>::type blockShape_;
    typename vigra::MultiArrayShape<N>::type shape_;
    std::map<BlockCoord, std::unique_ptr<CompressedArray<N, T> > > blocks_;
};

#endif /* BLOCKEDARRAY_H */