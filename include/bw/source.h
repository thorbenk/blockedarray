#ifndef BW_SOURCE_H
#define BW_SOURCE_H

#include <vigra/multi_array.hxx>

#include "roi.h"

namespace BW {
    
/**
 * Interface to obtain a block of data given a region of interest
 */
template<int N, class T>
class Source {
    public:
    typedef typename Roi<N>::V V;
        
    Source() {}
    virtual ~Source() {};
   
    /**
     * selects only the region of interest given from the
     * underlying data source. When readBlock() is used, the coordinates
     * are relative to roi.q
     */
    virtual void setRoi(Roi<N> roi) {};
    
    virtual V shape() const { return V(); };
    virtual bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const { return true; };
};

} /* namespace BW */

#endif /* BW_SOURCE_H */
