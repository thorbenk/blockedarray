#ifndef COMPRESSEDARRAY_H
#define COMPRESSEDARRAY_H

#include <boost/foreach.hpp>

#include <vigra/multi_array.hxx>

#include "snappy.h"

template<int N, class T>
class CompressedArray {
    public:
    
    typedef typename vigra::MultiArray<N, T>::difference_type difference_type;
    
    /**
     * construct a CompressedArray from the data 'a'
     * 
     * The array will be immediately stored compressed.
     */
    CompressedArray(const vigra::MultiArrayView<N, T, vigra::UnstridedArrayTag>& a)
        : data_(0)
        , isCompressed_(false)
        , compressedSizeBytes_(0)
        , shape_(a.shape())
        , isDirty_(false)
    {
        data_ = new char[a.size()];
        std::copy(a.begin(), a.end(), data_);

        compress();
    }

    /**
     * copy constructor
     */
    CompressedArray(const CompressedArray &other)
        : data_(0)
        , compressedSizeBytes_(other.compressedSizeBytes_)
        , isCompressed_(other.isCompressed)
        , shape_(other.shape_)
        , isDirty_(false)
    {
        data_ = new char[other.currentSizeBytes()];
        std::copy(other.data_, other.data_+other.currentSizeBytes(), data_);
    }
   
    /**
     * assignment operator
     */
    CompressedArray& operator=(const CompressedArray& other) {
        if (this != &other) {
            delete[] data_;
            data_ = 0;
            compressedSizeBytes_ = other.compressedSizeBytes_;
            isCompressed_ = other.isCompressed_;
            shape_ = other.shape_;
            isDirty_ = other.isDirty_;
            
            data_ = new char[other.currentSizeBytes()];
            std::copy(other.data_, other.data_+other.currentSizeBytes(), data_);
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
    void setDirty(bool dirty) { isDirty_ = dirty; }
    
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
    size_t compressedSize() const { return compressedSizeBytes_/sizeof(T); }

    /**
     * ensures that this array's data is stored uncompressed
     */
    void uncompress() {
        if(!isCompressed_) {
            return;
        }
        char* a = new char[uncompressedSize()*sizeof(T)];
        size_t r;
        snappy::GetUncompressedLength(data_, compressedSizeBytes_, &r);
        if(r != uncompressedSize()*sizeof(T)) {
            throw std::runtime_error("CompressedArray::uncompress: error");
        }
        snappy::RawUncompress(data_, compressedSizeBytes_, a);
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

        if(compressedSizeBytes_ == 0) {
            //this is the first time
            char* d = new char[snappy::MaxCompressedLength(uncompressedSizeBytes())];
            size_t outLength;
            snappy::RawCompress(data_, uncompressedSizeBytes(), d, &outLength);
            data_ = new char[outLength];
            std::copy(d, d+outLength, data_);
            delete[] d;
            isCompressed_ = true;
            compressedSizeBytes_ = outLength;
        }
        else {
            size_t outputLength;
            char* d = new char[compressedSizeBytes_];

            snappy::RawCompress(data_, uncompressedSizeBytes(), d, &outputLength);
            if(outputLength != compressedSizeBytes_) {
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
    vigra::MultiArray<N,T> readArray() const {
        vigra::MultiArray<N,T> a(shape_);
        if(isCompressed_) {
            snappy::RawUncompress((char*)data_, compressedSizeBytes_, (char*)a.data());
        }
        else {
            std::copy(data_, data_+uncompressedSize(), a.data());
        }
        return a;
    }
   
    /**
     * write the array 'a' into the region of interest [p,q)
     */
    void writeArray(difference_type p, difference_type q, const vigra::MultiArray<N,T>& a) {
        #ifdef DEBUG_CHECKS
        for(int k=0; k<N; ++k) {
            CHECK_OP(q[k]-p[k],==,a.shape(k)," ");
        }
        #endif
        bool wasCompressed = isCompressed_;
        if(isCompressed_) {
            uncompress();
        }
        compressedSizeBytes_ = 0; //we are writing new data, need to recompute compressed size
        vigra::MultiArrayView<N,T> oldA(shape_, (T*)data_);
        oldA.subarray(p,q) = a;
        if(wasCompressed) {
            compress();
        }
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
        return compressedSizeBytes_/((double)uncompressedSizeBytes());
    }

    private:
    char* data_;

    size_t compressedSizeBytes_;

    bool isCompressed_;
    typename vigra::MultiArray<N, T>::difference_type shape_;
    
    bool isDirty_;
};

#endif /* COMPRESSEDARRAY_H */