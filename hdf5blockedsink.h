#ifndef HDF5BLOCKEDSINK_H
#define HDF5BLOCKEDSINK_H

#include <vigra/hdf5impex.hxx>

#include "blockedsink.h"

/**
 * Write a block of data to a HDF5File
 */
template<int N, class T>
class HDF5BlockedSink : public BlockedSink<N,T> {
    public:
    typedef typename BlockedSink<N,T>::V V;

    HDF5BlockedSink(const std::string& hdf5file, const std::string& hdf5group, int compression = 1)
        : BlockedSink<N,T>()
        , hdf5file_(hdf5file)
        , hdf5group_(hdf5group)
        , compression_(compression)
        , fileCreated_(false)
    {
        vigra_precondition(compression >= 1 && compression <= 9, "compression must be >= 1 and <= 9");
    }

    virtual bool writeBlock(Roi<N> roi, const vigra::MultiArrayView<N,T>& block) {
        using namespace vigra;
        if(!fileCreated_) {
            if(this->shape() == V()) {
                throw std::runtime_error("HDF5BlockedSink: unknown shape");
            }
            if(this->blockShape() == V()) {
                this->setBlockShape(this->shape());
            }
            std::cout << "* write " << hdf5file_ << "/" << hdf5group_ << std::endl;
            HDF5File out(hdf5file_, HDF5File::Open);
            out.createDataset<N, T>(hdf5group_, this->shape(), 0, this->blockShape(), compression_);
            out.close();
            fileCreated_ = true;
        }
        
        HDF5File out(hdf5file_, HDF5File::Open);
        out.writeBlock(hdf5group_, roi.p, block);
        out.close();
    }
        
    private:
    std::string hdf5file_;
    std::string hdf5group_;
    int compression_;
    V shape_;
    bool fileCreated_;
};

#endif /* HDF5BLOCKEDSINK_H */
