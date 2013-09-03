#ifndef BW_BLOCKING_H
#define BW_BLOCKING_H

#include <boost/foreach.hpp>

#include <vector>
#include <iostream>

#include "roi.h"

namespace BW {

/**
 * Computes a tiling of (possibly overlapping) blocks.
 */
template<int N>
class Blocking {
    public:
    typedef typename Roi<N>::V V;
    
    typedef std::pair<V, Roi<N> > Pair;
        
    Blocking() {}
    
    Blocking(Roi<N> roi, V blockShape, V overlap = V() )
        : roi_(roi)
        , blockShape_(blockShape)
        , overlap_(overlap)
    {
        
        V blockP = blockGivenCoordinateP(roi.p);
        V blockQ = blockGivenCoordinateQ(roi.q);
    
        V x = roi.p;
        int dim=N-1;
        addBlock(x);
        while(dim >= 0) {
            while(dim >= 0) {
                if(x[dim] >= blockQ[dim]-1) {
                    x[dim] = blockP[dim];
                    --dim;
                    continue;
                } else {
                    ++x[dim];
                    addBlock(x);
                    break;
                }
            }
            if(dim >= 0) {
                dim = N-1;
            }
        }
    }
    
    size_t numBlocks() const {
        return blocks_.size();
    }
    
    void pprint() {
        std::pair<V, Roi<N> > roi;
        BOOST_FOREACH(roi, blocks_) {
            std::cout << roi.first << ": " << roi.second << std::endl;
        }
    }
    
    std::vector< std::pair<V, Roi<N> > > blocks() const { return blocks_; }
    
    private:
        
    V blockGivenCoordinateP(V p) const {
        V c;
        for(int i=0; i<N; ++i) { c[i] = p[i]/blockShape_[i]; }
        return c;
    }

    V blockGivenCoordinateQ(V q) const {
        V c;
        for(int i=0; i<N; ++i) { c[i] = (q[i]-1)/blockShape_[i] + 1; }
        return c;
    }
    
    void addBlock(V x) {
        Roi<N> r;
        for(int i=0; i<N; ++i) {
            r.p[i] = x[i]*blockShape_[i];
            r.q[i] = std::min( (x[i]+1)*blockShape_[i]+overlap_[i], roi_.q[i] );
        }
        blocks_.push_back(std::make_pair(x, r) );
    }

    Roi<N> roi_;
    V blockShape_;
    V overlap_;
    
    std::vector< std::pair<V, Roi<N> > > blocks_;
};

} /* namespace BW */

#endif /* BLCOKING_H */
