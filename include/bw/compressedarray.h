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
    
template<int N, class T>
class CompressedArray {
    public:
   
    //give unittest access
    friend class CompressedArrayTest<N, T>;
        
    typedef typename vigra::MultiArray<N, T>::difference_type difference_type;
    
    CompressedArray();
    
    /**
     * construct a CompressedArray from the data 'a'
     */
    CompressedArray(const vigra::MultiArrayView<N, T, vigra::UnstridedArrayTag>& a);

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
    bool isDirty() const;
    
    /**
     * set the dirty flag of this array 
     */
    void setDirty(bool dirty);
    
    bool isDirty(int dimension, int slice) const;
    
    bool isDirty(difference_type p, difference_type q) const;
    
    void setDirty(difference_type p, difference_type q, bool dirty);
    
    void setDirty(int dimension, int slice, bool dirty);
    
    /**
     * returns whether this array is currently stored in compressed form
     */
    bool isCompressed() const;

    /**
     * returns the uncompressed size (in number of elements T, _not_ in bytes)
     */
    size_t uncompressedSize() const;

    /**
     * returns the compressed size (in number of elements T, _not_ in bytes)
     */
    size_t compressedSize() const;

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
    void writeArray(difference_type p, difference_type q,
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
    
    difference_type shape() const;

    private:
    T*              data_;
    size_t          compressedSize_;
    bool            isCompressed_;
    difference_type shape_;
    bool            isDirty_;
    std::vector<bool> dirtyDimensions_;
};

//==== implementation ====//

template<int N, typename T>
CompressedArray<N,T>::CompressedArray()
    : data_(0)
    , isCompressed_(false)
    , compressedSize_(0)
    , isDirty_(false)
{}

template<int N, typename T>
CompressedArray<N,T>::CompressedArray(
    const vigra::MultiArrayView<N, T, vigra::UnstridedArrayTag>& a
)
    : data_(0)
    , isCompressed_(false)
    , compressedSize_(0)
    , shape_(a.shape())
    , isDirty_(false)
{
    data_ = new T[a.size()];
    std::copy(a.begin(), a.end(), data_);
    
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
bool CompressedArray<N,T>::operator==(const CompressedArray<N,T>& other) const {
    if(compressedSize_ != other.compressedSize_)   { return false; }
    if(isCompressed_ != other.isCompressed_)       { return false; }
    if(shape_ != other.shape_)                     { return false; }
    if(isDirty_ != other.isDirty_)                 { return false; }
    if(dirtyDimensions_ != other.dirtyDimensions_) { return false; }
    return std::equal(reinterpret_cast<char*>(data_),
                      reinterpret_cast<char*>(data_)+currentSizeBytes(),
                      reinterpret_cast<char*>(other.data_)); 
}

template<int N, typename T>
CompressedArray<N,T>::~CompressedArray() {
    delete[] data_;
}

template<int N, typename T>
bool CompressedArray<N,T>::isDirty() const { return isDirty_; }

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
void CompressedArray<N,T>::setDirty(difference_type p, difference_type q, bool dirty) {
    if(p == difference_type() && q == shape_) {
        //The whole block has been addressed.
        setDirty(dirty);
        return;
    }
    for(size_t d=0; d<N; ++d) { //go over all dimensions
        for(size_t i=p[d]; i<q[d]; ++i) { //in current dimension, go over the dimensions's range
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
bool CompressedArray<N,T>::isDirty(difference_type p, difference_type q) const {
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

template<int N, typename T>
bool CompressedArray<N,T>::isCompressed() const {
    return isCompressed_;
}

template<int N, typename T>
size_t CompressedArray<N,T>::uncompressedSize() const {
    size_t s = 1;
    BOOST_FOREACH(typename difference_type::size_type x, shape_) { s *= x; }
    return s;
}

template<int N, typename T>
size_t CompressedArray<N,T>::compressedSize() const {
    return compressedSize_;
}

template<int N, typename T>
void CompressedArray<N,T>::uncompress() {
    if(!isCompressed_) {
        return;
    }
    T* a = new T[uncompressedSize()];
    size_t r;
    snappy::GetUncompressedLength(reinterpret_cast<char*>(data_), compressedSize_*sizeof(T), &r);
    if(r != uncompressedSize()*sizeof(T)) {
        throw std::runtime_error("CompressedArray::uncompress: error");
    }
    snappy::RawUncompress(reinterpret_cast<char*>(data_), compressedSize_*sizeof(T), reinterpret_cast<char*>(a));
    delete[] data_;
    data_ = a;
    isCompressed_ = false;
}

template<int N, typename T>
void CompressedArray<N,T>::compress() {
    if(isCompressed_) {
        return;
    }

    if(compressedSize_ == 0) {
        //this is the first time
        
        const size_t M = snappy::MaxCompressedLength(uncompressedSizeBytes());
        size_t l = CEIL_INT_DIV(M, sizeof(T));
        T* d = new T[l];
        size_t outLength;
        snappy::RawCompress(reinterpret_cast<char*>(data_), uncompressedSizeBytes(),
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
        snappy::RawCompress(reinterpret_cast<char*>(data_), uncompressedSizeBytes(),
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
void CompressedArray<N,T>::readArray(vigra::MultiArray<N,T>& a) const {
    vigra_precondition(a.shape() == shape_, "shapes differ");
    if(isCompressed_) {
        size_t r;
        snappy::GetUncompressedLength(reinterpret_cast<char *>(data_), compressedSize_*sizeof(T), &r);
        if(r != uncompressedSize()*sizeof(T) || a.size()*sizeof(T) != r) {
            throw std::runtime_error("CompressedArray::uncompress: error");
        }
        snappy::RawUncompress(reinterpret_cast<char*>(data_), compressedSize_*sizeof(T), reinterpret_cast<char*>(a.data()));
    }
    else {
        std::copy(data_, data_+uncompressedSize(), a.data());
    }
}

template<int N, typename T>
void CompressedArray<N,T>::writeArray(
    difference_type p, difference_type q,
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
    compressedSize_ = 0; //we are writing new data, need to recompute compressed size
    vigra::MultiArrayView<N,T> oldA(shape_, (T*)data_);
    oldA.subarray(p,q) = a;
    if(wasCompressed) {
        compress();
    }

    //
    //keep track of dirtyness
    //
    
    if(p == difference_type() && q == shape_) {
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

template<int N, typename T>
size_t CompressedArray<N,T>::currentSize() const {
    if(isCompressed_) {
        return compressedSize_;
    }
    return uncompressedSize();
}

template<int N, typename T>
size_t CompressedArray<N,T>::currentSizeBytes() const {
    if(isCompressed_) {
        return compressedSize() * sizeof(T);
    }
    return uncompressedSize() * sizeof(T);
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

template<int N, typename T>
typename CompressedArray<N,T>::difference_type
CompressedArray<N,T>::shape() const {
    return shape_;
}

template<int N, typename T>
void CompressedArray<N,T>::writeHDF5(
    hid_t group,
    const char* name
) const {
    hsize_t size = currentSizeBytes();
  
    hid_t dataspace;
    if(size>  0) { 
        dataspace = H5Screate_simple(1, &size, NULL); 
    }
    else {
        hsize_t one = 1;
        dataspace = H5Screate_simple(1, &one, NULL); 
    }
    hid_t dataset   = H5Dcreate(group, name, H5T_STD_U8LE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5A<bool>::write(dataset, "empty", size == 0);
   
    if(size > 0) { 
	H5Dwrite(dataset, H5T_STD_U8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_);
    }
  
    //dirtyDimensions_
    {
        hsize_t dsSize = dirtyDimensions_.size();
        if(dsSize > 0) {
            unsigned char* ds = new unsigned char[dsSize];
            std::copy(dirtyDimensions_.begin(), dirtyDimensions_.end(), ds);
            hid_t space    = H5Screate_simple(1, &dsSize, NULL);
            hid_t attr     = H5Acreate(dataset, "ds", H5T_STD_U8LE, space, H5P_DEFAULT, H5P_DEFAULT);
            H5Awrite(attr, H5T_NATIVE_UINT8, ds);
            H5Aclose(attr);
            H5Sclose(space);
            delete[] ds;
        }
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
        hid_t attr     = H5Acreate(dataset, "sh", H5T_STD_U32LE, space, H5P_DEFAULT, H5P_DEFAULT);
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
            H5Dread(dataset, H5T_STD_U8LE /*memtype*/, H5S_ALL, H5S_ALL, H5P_DEFAULT, reinterpret_cast<unsigned char*>(ca.data_));
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

