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

#ifndef BW_BLOCKEDSINK_H
#define BW_BLOCKEDSINK_H

#include <vigra/multi_array.hxx>

#include "roi.h"

namespace BW {

/**
 * Interface to write a block of data given a region of interest
 */
template<int N, class T>
class Sink {
    public:
    typedef typename Roi<N>::V V;

    Sink() {}
    virtual ~Sink() {};

    void setShape(V shape) {
        shape_ = shape;
    }

    void setBlockShape(V shape) {
        blockShape_ = shape;
    }

    V shape() const { return shape_; };
    V blockShape() const { return blockShape_; };

    virtual bool writeBlock(Roi<N> roi, const vigra::MultiArrayView<N,T>& block) { return true; };

    protected:
    V shape_;
    V blockShape_;
};

} /* namespace BW */

#endif /* BW_BLOCKEDSINK_H */
