#ifndef COMPRESSEDARRAY_H
#define COMPRESSEDARRAY_H

#include "snappy.h"

#include <vigra/multi_array.hxx>

template<int N, class T>
class CompressedArray {
    public:
    
    typedef typename vigra::MultiArray<N, T>::difference_type difference_type;
        
    CompressedArray(const vigra::MultiArrayView<N, T, vigra::UnstridedArrayTag>& a)
        : data_(0), isCompressed_(false), compressedSizeBytes_(0), shape_(a.shape()) {
        data_ = new char[a.size()];
        std::copy(a.begin(), a.end(), data_);

        compress();
    }

    ~CompressedArray() {
        delete[] data_;
    }

    bool isCompressed() const { return isCompressed_; }

    size_t uncompressedSize() const {
        size_t s = 1;
        for(auto x : shape_) { s *= x; }
        return s;
    }

    size_t compressedSize() const { return compressedSizeBytes_/sizeof(T); }

    void uncompress() {
        if(!isCompressed_) {
            return;
        }
        char* a = new char[size()*sizeof(T)];
        size_t r;
        snappy::GetUncompressedLength(data_, compressedSizeBytes_, &r);
        if(r != size()*sizeof(T)) {
            throw std::runtime_error("bad bad bad");
        }
        snappy::RawUncompress(data_, compressedSizeBytes_, a);
        delete[] data_;
        data_ = a;
        isCompressed_ = false;
    }

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
            if(outputLength != compressedSizeBytes_) { throw std::runtime_error("bad"); }
            delete[] data_;
            data_ = d;
            isCompressed_ = true;
        }
    }

    vigra::MultiArray<N,T> readArray() const {
        vigra::MultiArray<N,T> a(shape_);
        if(isCompressed_) {
            snappy::RawUncompress((char*)data_, compressedSizeBytes_, (char*)a.data());
        }
        else {
            std::copy(data_, data_+size(), a.data());
        }
        return a;
    }
    
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

    size_t size() const {
        size_t ret = 1;
        for(auto x: shape_) {
            ret *= x;
        }
        return ret;
    }

    size_t currentSizeBytes() const {
        if(isCompressed_) {
            return compressedSize() * sizeof(T);
        }
        return uncompressedSize() * sizeof(T);
    }
    size_t uncompressedSizeBytes() const {
        return size()*sizeof(T);
    }

    double compressionRatio() const {
        return compressedSizeBytes_/((double)uncompressedSizeBytes());
    }

    private:
    char* data_;

    size_t compressedSizeBytes_;

    bool isCompressed_;
    typename vigra::MultiArray<N, T>::difference_type shape_;
};

#endif /* COMPRESSEDARRAY_H */