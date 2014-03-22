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

#ifndef BW_ROI_H
#define BW_ROI_H

#include <vigra/tinyvector.hxx>
#include <vigra/multi_shape.hxx>

namespace BW {

/**
 * Region of interest
 *
 * A ROI is defined by a (N-dimensional) rectangular region.
 * The lower left corner is Roi::p, the upper right cornver Roi::q
 * (with Roi::q being exclusive).
 */
template<int N>
class Roi {
    public:
    typedef vigra::TinyVector<vigra::MultiArrayIndex, N> V;

    /**
     * Region of interest [start, end)
     */
    Roi(V start, V end) : p(start), q(end) {}

    /**
     * Constructs an empty region of interest
     */
    Roi() {}

    /**
     * equality
     */
    bool operator==(const Roi<N>& other) const {
        return other.p == p && other.q == q;
    }

    /**
     * inequality
     */
    bool operator!=(const Roi<N>& other) const {
        return other.p != p || other.q != q;
    }

    /**
     * shift roi by 'shift'
     */
    const Roi<N> operator+(const V& shift) const {
        Roi<N> res = *this;
        res.p += shift;
        res.q += shift;
        return res;
    }

    /**
     * shift roi by 'shift'
     */
    Roi<N>& operator+=(const V& shift) {
        p += shift;
        q += shift;
        return *this;
    }

    /**
     * intersection of this Roi with 'other'
     *
     * if an intersection exists, it is written into 'out'
     *
     * returns: whether this roi and 'other' intersect.
     */
    bool intersect(const Roi& other, Roi& out) const {
        for(int i=0; i<N; ++i) {
            out.p[i] = std::max(p[i], other.p[i]);
            out.q[i] = std::min(q[i], other.q[i]);
            if(out.p[i] >= q[i] || out.p[i] >= other.q[i] || out.q[i] < p[i] || out.q[i] < other.p[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * extent of this region of interest
     */
    V shape() const {
        return q-p;
    }

    size_t size() const {
        size_t ret = 1;
        V sh = shape();
        for(int i=0; i<N; ++i) {
            ret *= sh[i];
        }
        return ret;
    }

    /**
     * remove the 'axis'-th dimension (counting from 0)
     */
    Roi<N-1> removeAxis(int axis) const {
        Roi<N-1> ret;
        int j=0;
        for(int i=0; i<N; ++i) {
            if(i == axis) continue;
            ret.p[j] = p[i];
            ret.q[j] = q[i];
            ++j;
        }
        return ret;
    }

    /**
     * append a dimension/axis at the end, for which the region of interest
     * lies in [from, to)
     */
    Roi<N+1> appendAxis(int from, int to) const {
        Roi<N+1> ret;
        std::copy(p.begin(), p.end(), ret.p.begin());
        std::copy(q.begin(), q.end(), ret.q.begin());
        ret.p[N] = from;
        ret.q[N] = to;
        return ret;
    }

    /**
     * prepend a dimension/axis at the beginning, for which the region of interest
     * lies in [from, to)
     */
    Roi<N+1> prependAxis(int from, int to) const {
        Roi<N+1> ret;
        std::copy(p.begin(), p.end(), ret.p.begin()+1);
        std::copy(q.begin(), q.end(), ret.q.begin()+1);
        ret.p[0] = from;
        ret.q[0] = to;
        return ret;
    }

    Roi<N+1> insertAxisBefore(int dim, int from, int to) const {
        Roi<N+1> ret;
        int j = 0; //old dim
        for(int i=0; i<N+1; ++i) { //new dimensions
            if(i == dim) {
                ret.p[i] = from;
                ret.q[i] = to;

            }
            else {
                ret.p[i] = p[j];
                ret.q[i] = q[j];
                ++j;
            }
        }
        return ret;
    }
    
    V getP()
    {
        return p;
    }
    
    V getQ()
    {
        return q;
    }

    V p;
    V q;
};

template<int N>
std::ostream& operator<<(std::ostream& o, const Roi<N>& roi) {
    o << roi.p << " -- " << roi.q;
    return o;
}

} /* namespace BW */

#endif /* BW_ROI_H */
