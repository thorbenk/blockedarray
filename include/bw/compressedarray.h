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

namespace BW {

template<int N, class T>
class CompressedArray {
    public:
    
    typedef typename vigra::MultiArray<N, T>::difference_type difference_type;
    
    CompressedArray()
        : data_(0)
        , isCompressed_(false)
        , compressedSize_(0)
        , isDirty_(false)
    {}
    
    /**
     * construct a CompressedArray from the data 'a'
     */
    CompressedArray(const vigra::MultiArrayView<N, T, vigra::UnstridedArrayTag>& a)
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

    /**
     * copy constructor
     */
    CompressedArray(const CompressedArray &other)
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
   
    /**
     * assignment operator
     */
    CompressedArray& operator=(const CompressedArray& other) {
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

    /**
     * destructor
     */
    ~CompressedArray() {
        delete[] data_;
    }
   
    /**
     * returns whether this array is marked as dirty 
     */
    bool isDirty() const { return isDirty_; }
    
    /**
     * set the dirty flag of this array 
     */
    void setDirty(bool dirty) { 
        isDirty_ = dirty;
        std::fill(dirtyDimensions_.begin(), dirtyDimensions_.end(), dirty);
    }
    
    bool isDirty(int dimension, int slice) const {
        size_t n=0;
        for(int d=0; d<dimension-1; ++d) n += shape_[d];
        return dirtyDimensions_[n+slice];
    }
    
    void setDirty(int dimension, int slice, bool dirty) {
        size_t n=0;
        for(int d=0; d<dimension-1; ++d) n += shape_[d];
        dirtyDimensions_[n+slice] = dirty;
    }
    
    /**
     * returns whether this array is currently stored in compressed form
     */
    bool isCompressed() const { return isCompressed_; }

    /**
     * returns the uncompressed size (in number of elements T, _not_ in bytes)
     */
    size_t uncompressedSize() const {
        size_t s = 1;
        BOOST_FOREACH(typename difference_type::size_type x, shape_) { s *= x; }
        return s;
    }

    /**
     * returns the compressed size (in number of elements T, _not_ in bytes)
     */
    size_t compressedSize() const { return compressedSize_; }

    /**
     * ensures that this array's data is stored uncompressed
     */
    void uncompress() {
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

    /**
     * ensures that this array's data is stored compressed
     */
    void compress() {
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

    /**
     * returns (potentially after uncompressing) this array's data
     */
    void readArray(vigra::MultiArray<N,T>& a) const {
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
   
    /**
     * write the array 'a' into the region of interest [p,q)
     */
    void writeArray(difference_type p, difference_type q, const vigra::MultiArrayView<N,T>& a) {
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
    
    /**
     * returns the size of the array (in elements T)
     */
    size_t currentSize() const {
        if(isCompressed_) {
            return compressedSize_;
        }
        return uncompressedSize();
    }

    /**
     * returns the size of the array in memory (in bytes)
     */
    size_t currentSizeBytes() const {
        if(isCompressed_) {
            return compressedSize() * sizeof(T);
        }
        return uncompressedSize() * sizeof(T);
    }
    
    /**
     * returns the size of the array when uncompressed (in bytes)
     */
    size_t uncompressedSizeBytes() const {
        return uncompressedSize()*sizeof(T);
    }

    /**
     * returns the compression ratio
     * 
     * Note that this is only valid if compress() has been called at least once on the
     * current data.
     */
    double compressionRatio() const {
        return compressedSize_*sizeof(T)/((double)uncompressedSizeBytes());
    }
    
    difference_type shape() const { return shape_; }

    private:
    T*              data_;
    size_t          compressedSize_;
    bool            isCompressed_;
    difference_type shape_;
    bool            isDirty_;
    std::vector<bool> dirtyDimensions_;
};

} /* namespace BW */

#endif /* BW_COMPRESSEDARRAY_H */

