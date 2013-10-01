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
