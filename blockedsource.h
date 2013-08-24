#ifndef BLOCKEDSOURCE_H
#define BLOCKEDSOURCE_H

#include <vigra/multi_array.hxx>

#include "roi.h"

/**
 * Interface to obtain a block of data given a region of interest
 */
template<int N, class T>
class BlockedSource {
    public:
    typedef typename Roi<N>::V V;
        
    BlockedSource() {}
    virtual ~BlockedSource() {};
   
    /**
     * selects only the region of interest given from the
     * underlying data source. When readBlock() is used, the coordinates
     * are relative to roi.q
     */
    virtual void setRoi(Roi<N> roi) {};
    
    virtual V shape() const { return V(); };
    virtual bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const { return true; };
};

#endif /* BLOCKEDSOURCE_H */
