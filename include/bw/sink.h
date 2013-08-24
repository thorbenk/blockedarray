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
