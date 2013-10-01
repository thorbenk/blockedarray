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

#ifndef BW_COMPRESSEDARRAY_H
#define BW_COMPRESSEDARRAY_H

#include <boost/foreach.hpp>

#include <vigra/multi_array.hxx>

#include "snappy.h"

#define CEIL_INT_DIV(a, b) ((a+b-1)/b)

#ifdef DEBUG_CHECKS
#define CHECK_OP(a,op,b,message) \
    if(!  static_cast<bool>( a op b )   ) { \
       std::stringstream s; \
       s << "Error: "<< message <<"\n";\
       s << "check :  " << #a <<#op <<#b<< "  failed:\n"; \
       s << #a " = "<<a<<"\n"; \
       s << #b " = "<<b<<"\n"; \
       s << "in file " << __FILE__ << ", line " << __LINE__ << "\n"; \
       throw std::runtime_error(s.str()); \
    }
#endif

#include <bw/hdf5utils.h>

#include <iostream>

template<int Dim, class Type>
class CompressedArrayTest;

namespace BW {

/**
 * A multidimensional array which supports in-memory compression.
 *
 * CompressedArray<N,T> is an array of dimension N and voxel type T.
 * It supports in memory compression using the google snappy compression
 * algorithm.
 */
template<int N, class T>
class CompressedArray {
    public:

    //give unittest access
    friend class CompressedArrayTest<N, T>;

    typedef typename vigra::MultiArray<N, T>::difference_type V;

    CompressedArray();

    /**
     * construct a CompressedArray from the data 'a'
     */
    template <typename StrideTag>
    CompressedArray(const vigra::MultiArrayView<N, T, StrideTag>& a);

    /**
     * copy constructor
     */
    CompressedArray(const CompressedArray &other);

    /**
     * assignment operator
     */
    CompressedArray& operator=(const CompressedArray& other);

    bool operator==(const CompressedArray<N,T>& other) const;

    /**
     * destructor
     */
    ~CompressedArray();

    static CompressedArray<N,T> readHDF5(hid_t group, const char* name);

    void writeHDF5(hid_t group, const char* name) const;

    /**
     * returns whether this array is marked as dirty
     */
    bool isDirty() const { return isDirty_; }

    /**
     * set the dirty flag of this array
     */
    void setDirty(bool dirty);

    bool isDirty(int dimension, int slice) const;

    bool isDirty(V p, V q) const;

    void setDirty(V p, V q, bool dirty);

    void setDirty(int dimension, int slice, bool dirty);

    /**
     * returns whether this array is currently stored in compressed form
     */
    bool isCompressed() const { return isCompressed_; }

    /**
     * returns the uncompressed size (in number of elements T, _not_ in bytes)
     */
    size_t uncompressedSize() const;

    /**
     * returns the compressed size (in number of elements T, _not_ in bytes)
     */
    size_t compressedSize() const { return compressedSize_; }

    /**
     * ensures that this array's data is stored uncompressed
     */
    void uncompress();

    /**
     * ensures that this array's data is stored compressed
     */
    void compress();

    /**
     * returns (potentially after uncompressing) this array's data
     */
    void readArray(vigra::MultiArray<N,T>& a) const;

    /**
     * write the array 'a' into the region of interest [p,q)
     */
    void writeArray(V p, V q,
                    const vigra::MultiArrayView<N,T>& a);

    /**
     * returns the size of the array (in elements T)
     */
    size_t currentSize() const;

    /**
     * returns the size of the array in memory (in bytes)
     */
    size_t currentSizeBytes() const;

    /**
     * returns the size of the array when uncompressed (in bytes)
     */
    size_t uncompressedSizeBytes() const;

    /**
     * returns the compression ratio
     *
     * If this array is compressed, returns the compression ratio.
     * If this array is uncompressed, return 1.0
     */
    double compressionRatio() const;

    V shape() const { return shape_; }

    private:
    T*                data_;
    size_t            compressedSize_;
    bool              isCompressed_;
    V                 shape_;
    bool              isDirty_;
    std::vector<bool> dirtyDimensions_;
};

//==========================================================================//
// Constructors, Destructor                                                 //
//==========================================================================//

template<int N, typename T>
CompressedArray<N,T>::CompressedArray()
    : data_(0)
    , isCompressed_(false)
    , compressedSize_(0)
    , isDirty_(false)
{}

template<int N, typename T>
template <typename StrideTag>
CompressedArray<N,T>::CompressedArray(
    const vigra::MultiArrayView<N, T, StrideTag>& a
)
    : data_(0)
    , isCompressed_(false)
    , compressedSize_(0)
    , shape_(a.shape())
    , isDirty_(false)
{
    data_ = new T[a.size()];

    // MultiArrayView assign is faster than std::copy(a.begin(), a.end(), data_)
    // (By 2x on my machine!)
    vigra::MultiArrayView<N,T> mydata(shape_, (T*)data_);
    mydata = a;

    size_t n = 0;
    for(int d=0; d<N; ++d) {
        n += a.shape(d);
    }
    dirtyDimensions_.resize(n);
}

template<int N, typename T>
CompressedArray<N,T>::CompressedArray(const CompressedArray<N,T> &other)
    : data_(0)
    , compressedSize_(other.compressedSize_)
    , isCompressed_(other.isCompressed_)
    , shape_(other.shape_)
    , isDirty_(false)
    , dirtyDimensions_(other.dirtyDimensions_)
{
    data_ = new T[other.currentSize()];
    std::copy(other.data_, other.data_+other.currentSize(), data_);
}

template<int N, typename T>
CompressedArray<N,T>::~CompressedArray() {
    delete[] data_;
}

//==========================================================================//
// operator=, operator==                                                    //
//==========================================================================//

template<int N, typename T>
CompressedArray<N,T>& CompressedArray<N,T>::operator=(
    const CompressedArray& other
) {
    if (this != &other) {
        delete[] data_;
        data_ = 0;
        compressedSize_ = other.compressedSize_;
        isCompressed_ = other.isCompressed_;
        shape_ = other.shape_;
        isDirty_ = other.isDirty_;
        dirtyDimensions_ = other.dirtyDimensions_;

        data_ = new T[other.currentSize()];
        std::copy(other.data_, other.data_+other.currentSize(), data_);
    }
    return *this;
}

template<int N, typename T>
bool CompressedArray<N,T>::operator==(
    const CompressedArray<N,T>& other
) const {
    if(compressedSize_ != other.compressedSize_)   { return false; }
    if(isCompressed_ != other.isCompressed_)       { return false; }
    if(shape_ != other.shape_)                     { return false; }
    if(isDirty_ != other.isDirty_)                 { return false; }
    if(dirtyDimensions_ != other.dirtyDimensions_) { return false; }
    return std::equal(reinterpret_cast<char*>(data_),
                      reinterpret_cast<char*>(data_)+currentSizeBytes(),
                      reinterpret_cast<char*>(other.data_));
}

//==========================================================================//
// dirtyness                                                                //
//==========================================================================//

template<int N, typename T>
void CompressedArray<N,T>::setDirty(bool dirty) {
    isDirty_ = dirty;
    std::fill(dirtyDimensions_.begin(), dirtyDimensions_.end(), dirty);
}

template<int N, typename T>
bool CompressedArray<N,T>::isDirty(int dimension, int slice) const {
    size_t n=0;
    for(int d=0; d<dimension-1; ++d) n += shape_[d];
    return dirtyDimensions_[n+slice];
}

template<int N, typename T>
void CompressedArray<N,T>::setDirty(V p, V q, bool dirty) {
    if(p == V() && q == shape_) {
        //The whole block has been addressed.
        setDirty(dirty);
        return;
    }
    for(size_t d=0; d<N; ++d) { //go over all dimensions
        //in current dimension, go over the dimensions's range
        for(size_t i=p[d]; i<q[d]; ++i) {
            if(dirty) {
                setDirty(d, i, true);
                continue;
            }
            //go over all other dimensions
            bool sliceFull = true;
            for(size_t j=0; j<N; ++j) {
                if(j==d) continue;
                if(p[j] != 0 || q[j] != shape_[j]) {
                    sliceFull = false;
                    break;
                }
            }
            if(sliceFull) {
                setDirty(d, i, false);
            }
        }
    }
}

template<int N, typename T>
bool CompressedArray<N,T>::isDirty(V p, V q) const {
    for(size_t d=0; d<N; ++d) {
        bool dirty = false;
        for(size_t i=p[d]; i<q[d]; ++i) {
            if(isDirty(d, i)) {
                dirty = true;
                break;
            }
        }
        if(!dirty) {
            return false;
        }
    }
    return true;
}

template<int N, typename T>
void CompressedArray<N,T>::setDirty(int dimension, int slice, bool dirty) {
    size_t n=0;
    for(int d=0; d<dimension-1; ++d) n += shape_[d];
    dirtyDimensions_[n+slice] = dirty;
}

//==========================================================================//
// compression & (byte) size                                                //
//==========================================================================//

template<int N, typename T>
size_t CompressedArray<N,T>::uncompressedSize() const {
    size_t s = 1;
    BOOST_FOREACH(typename V::size_type x, shape_) { s *= x; }
    return s;
}

template<int N, typename T>
void CompressedArray<N,T>::uncompress() {
    using namespace snappy;
    if(!isCompressed_) return;

    T* a = new T[uncompressedSize()];
    size_t r;
    GetUncompressedLength(reinterpret_cast<char*>(data_),
                          compressedSize_*sizeof(T), &r);
    if(r != uncompressedSize()*sizeof(T)) {
        throw std::runtime_error("CompressedArray::uncompress: error");
    }
    RawUncompress(reinterpret_cast<char*>(data_), compressedSize_*sizeof(T),
                  reinterpret_cast<char*>(a));
    delete[] data_;
    data_ = a;
    isCompressed_ = false;
}

template<int N, typename T>
void CompressedArray<N,T>::compress() {
    if(isCompressed_) return;

    using namespace snappy;
    if(compressedSize_ == 0) {
        //this is the first time

        const size_t M = snappy::MaxCompressedLength(uncompressedSizeBytes());
        size_t l = CEIL_INT_DIV(M, sizeof(T));
        T* d = new T[l];
        size_t outLength;
        RawCompress(reinterpret_cast<char*>(data_), uncompressedSizeBytes(),
                    reinterpret_cast<char*>(d), &outLength);
        outLength = CEIL_INT_DIV(outLength, sizeof(T));
        delete[] data_;
        data_ = new T[outLength];
        std::copy(d, d+outLength, data_);
        delete[] d;
        isCompressed_ = true;
        compressedSize_ = outLength;
    }
    else {
        size_t outputLength;
        T* d = new T[compressedSize_];
        RawCompress(reinterpret_cast<char*>(data_), uncompressedSizeBytes(),
                    reinterpret_cast<char *>(d), &outputLength);
        if(CEIL_INT_DIV(outputLength, sizeof(T)) != compressedSize_) {
            throw std::runtime_error("CompressedArray::compress error");
        }
        delete[] data_;
        data_ = d;
        isCompressed_ = true;
    }
}

template<int N, typename T>
size_t CompressedArray<N,T>::currentSize() const {
    return isCompressed_ ? compressedSize_ : uncompressedSize();
}

template<int N, typename T>
size_t CompressedArray<N,T>::currentSizeBytes() const {
    return isCompressed_ ? compressedSize()*sizeof(T)
                         : uncompressedSize()*sizeof(T);
}

template<int N, typename T>
size_t CompressedArray<N,T>::uncompressedSizeBytes() const {
    return uncompressedSize()*sizeof(T);
}

template<int N, typename T>
double CompressedArray<N,T>::compressionRatio() const {
    if(isCompressed_) {
        return compressedSize_*sizeof(T)/((double)uncompressedSizeBytes());
    }
    return 1.0;
}

//==========================================================================//
// read data                                                                //
//==========================================================================//

template<int N, typename T>
void CompressedArray<N,T>::readArray(vigra::MultiArray<N,T>& a) const {
    using namespace snappy;
    vigra_precondition(a.shape() == shape_, "shapes differ");
    if(isCompressed_) {
        size_t r;
        GetUncompressedLength(reinterpret_cast<char *>(data_),
                              compressedSize_*sizeof(T), &r);
        if(r != uncompressedSize()*sizeof(T) || a.size()*sizeof(T) != r) {
            throw std::runtime_error("CompressedArray::uncompress: error");
        }
        RawUncompress(reinterpret_cast<char*>(data_),
                      compressedSize_*sizeof(T),
                      reinterpret_cast<char*>(a.data()));
    }
    else {
        vigra::MultiArrayView<N,T> mydata(shape_, (T*)data_);
        a = mydata;
    }
}

//==========================================================================//
// write data                                                               //
//==========================================================================//

template<int N, typename T>
void CompressedArray<N,T>::writeArray(
    V p, V q,
    const vigra::MultiArrayView<N,T>& a
) {
    #ifdef DEBUG_CHECKS
    for(int k=0; k<N; ++k) {
        CHECK_OP(q[k]-p[k],==,a.shape(k)," ");
    }
    #endif
    bool wasCompressed = isCompressed_;
    if(isCompressed_) {
        uncompress();
    }
    //we are writing new data, need to recompute compressed size
    compressedSize_ = 0;
    vigra::MultiArrayView<N,T> oldA(shape_, (T*)data_);
    oldA.subarray(p,q) = a;
    if(wasCompressed) {
        compress();
    }

    //keep track of dirtyness
    if(p == V() && q == shape_) {
        //the whole block gets overwritten
        setDirty(false);
        std::fill(dirtyDimensions_.begin(), dirtyDimensions_.end(), false);
    }

    for(int d=0; d<N; ++d) {
        //all dimension (except d) should have full extent
        bool fullExtent = true;
        for(int dim=0; dim<N; ++dim) {
            if(dim == d) continue;
            fullExtent &= (p[dim] == 0 && q[dim] == shape_[dim]);
        }
        if(fullExtent) {
            bool allClean = true;
            for(int s=0; s<shape_[d]; ++s) {
                bool sliceClean = (s>=p[d] && s<q[d]);
                if(sliceClean) {
                    setDirty(d, s, false);
                }
                allClean &= !isDirty(d,s);
            }
            if(allClean) {
                std::fill(dirtyDimensions_.begin(), dirtyDimensions_.end(), false);
                setDirty(false);
                break;
            }
        }
    }
}

//==========================================================================//
// HDF5                                                                     //
//==========================================================================//

template<int N, typename T>
void CompressedArray<N,T>::writeHDF5(
    hid_t group,
    const char* name
) const {
    hsize_t size = currentSizeBytes();

    hid_t dataspace;
    if(size>  0) { dataspace = H5Screate_simple(1, &size, NULL); }
    else {
        hsize_t one = 1;
        dataspace = H5Screate_simple(1, &one, NULL);
    }
    hid_t dataset = H5Dcreate(group, name, H5T_STD_U8LE, dataspace,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5A<bool>::write(dataset, "empty", size == 0);
    if(size > 0) {
        H5Dwrite(dataset, H5T_STD_U8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_);
    }

    if(dirtyDimensions_.size() > 0) {
        hsize_t dsSize = dirtyDimensions_.size();
        unsigned char* ds = new unsigned char[dsSize];
        std::copy(dirtyDimensions_.begin(), dirtyDimensions_.end(), ds);
        hid_t space    = H5Screate_simple(1, &dsSize, NULL);
        hid_t attr     = H5Acreate(dataset, "ds", H5T_STD_U8LE, space,
                                   H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, H5T_NATIVE_UINT8, ds);
        H5Aclose(attr);
        H5Sclose(space);
        delete[] ds;
    }
    H5A<size_t>::write(dataset, "cs", compressedSize_);
    H5A<bool>::write(dataset, "d", isDirty_);
    H5A<bool>::write(dataset, "c", isCompressed_);
    //shape_;
    {
        hsize_t n = N;
        uint32_t* sh = new uint32_t[N];
        std::copy(shape_.begin(), shape_.end(), sh);

        hid_t space    = H5Screate_simple(1, &n, NULL);
        hid_t attr     = H5Acreate(dataset, "sh", H5T_STD_U32LE, space,
                                   H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, H5T_NATIVE_UINT32, sh);

        H5Aclose(attr);
        H5Sclose(space);

        delete[] sh;
    }

    H5Dclose(dataset);
    H5Sclose(dataspace);
}

template<int N, typename T>
CompressedArray<N,T>
CompressedArray<N,T>::readHDF5(
    hid_t group,
    const char* name
) {
    CompressedArray<N,T> ca;

    hid_t dataset   = H5Dopen(group, name, H5P_DEFAULT);
    hid_t filespace = H5Dget_space(dataset);

    hsize_t dataSize;

    ca.compressedSize_ = H5A<size_t>::read(dataset, "cs");

    bool empty = false;
    //data_
    {
        H5Sget_simple_extent_dims(filespace, &dataSize, NULL);
        if(dataSize == 1) {
	    empty = H5A<bool>::read(dataset, "empty");
        }
        if(!empty) {
            ca.data_ = new T[dataSize/sizeof(T)];
            H5Dread(dataset, H5T_STD_U8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                    reinterpret_cast<unsigned char*>(ca.data_));
        }
    }
    //dirtyDimensions_
    if(H5Aexists(dataset, "ds")) {
        hid_t attr  = H5Aopen(dataset, "ds", H5P_DEFAULT);
        hid_t space = H5Aget_space(attr);

        hsize_t dim;
        H5Sget_simple_extent_dims(space, &dim, NULL);

        unsigned char* ds = new unsigned char[dim];
        H5Aread(attr, H5T_NATIVE_UINT8, ds);
        ca.dirtyDimensions_.resize(dim);
        std::copy(ds, ds+dim, ca.dirtyDimensions_.begin());
        delete[] ds;
        H5Sclose(space);
        H5Aclose(attr);
    }

    ca.isDirty_      = H5A<bool>::read(dataset, "d");
    ca.isCompressed_ = H5A<bool>::read(dataset, "c");

    //shape_
    {
        hid_t attr = H5Aopen(dataset, "sh", H5P_DEFAULT);
        uint32_t sh[N];
        H5Aread(attr, H5T_NATIVE_UINT32 /*memtype*/, sh);
        H5Aclose(attr);
        std::copy(sh, sh+N, ca.shape_.begin());
    }

    H5Dclose(dataset);
    H5Sclose(filespace);

    return ca;
}

} /* namespace BW */

#endif /* BW_COMPRESSEDARRAY_H */

