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

#ifndef BW_SINKHDF5_H
#define BW_SINKHDF5_H

#include <vigra/hdf5impex.hxx>

#include <bw/sink.h>

namespace BW {

/**
 * Write a block of data to a HDF5File
 */
template<int N, class T>
class SinkHDF5 : public Sink<N,T> {
    public:
    typedef typename Sink<N,T>::V V;

    SinkHDF5(const std::string& hdf5file, const std::string& hdf5group, int compression = 1)
        : Sink<N,T>()
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
                throw std::runtime_error("SinkHDF5: unknown shape");
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

        return true;
    }

    private:
    std::string hdf5file_;
    std::string hdf5group_;
    int compression_;
    V shape_;
    bool fileCreated_;
};

} /* namespace BW */

#endif /* BW_SINKHDF5_H */
