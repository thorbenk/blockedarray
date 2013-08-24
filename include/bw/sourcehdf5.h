#ifndef BW_SOURCEHDF5_H
#define BW_SOURCEHDF5_H 

#include <vigra/hdf5impex.hxx>

#include <bw/source.h>

namespace BW {
    
/**
 * Read a block of data from a HDF5File
 */
template<int N, class T>
class SourceHDF5 : public Source<N,T> {
    public:
    typedef typename Source<N,T>::V V;
    
    SourceHDF5(const std::string& hdf5file, const std::string& hdf5group)
        : Source<N,T>()
        , hdf5file_(hdf5file)
        , hdf5group_(hdf5group)
    {
    }
    
    virtual void setRoi(Roi<N> roi) {
       roi_ = roi; 
    }
    
    virtual V shape() const {
        using namespace vigra;
        
        V ret;
        HDF5File f(hdf5file_, HDF5File::OpenReadOnly);
        vigra::ArrayVector<hsize_t> sh = f.getDatasetShape(hdf5group_);
        f.close();
        vigra_precondition(sh.size() == N, "dataset shape is wrong");
        std::copy(sh.begin(), sh.end(), ret.begin());
        if(roi_ != Roi<N>()) {
            Roi<N> in(V(), ret);
            Roi<N> out;
            in.intersect(roi_, out);
            return out.shape();
        }
        return ret;
    }
    
    virtual bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const {
        using namespace vigra;
        
        HDF5File in(hdf5file_, HDF5File::OpenReadOnly);
        if(roi_ != Roi<N>()) {
            roi += roi_.p;
            Roi<N> newRoi;
            roi_.intersect(roi, newRoi);
            roi = newRoi;
        }
        vigra_precondition(roi.shape() == block.shape(), "shapes differ");
        
        in.readBlock(hdf5group_, roi.p, roi.q-roi.p, block);
        in.close();
        return true;
    }
    
    private:
    std::string hdf5file_;
    std::string hdf5group_;
    Roi<N> roi_;
};

} /* namespace BW */

#endif /* BW_SOURCEHDF5_H */
