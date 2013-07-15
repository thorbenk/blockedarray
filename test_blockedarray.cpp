#include <vector>
#include <iostream>
#include <memory>
#include <map>
#include <chrono>
#include <cmath>

#include <vigra/multi_array.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>
#include <vigra/random.hxx>

#include "blockedarray.h"

//TODO: use alignas to allocate char arrays

#undef DEBUG_CHECKS
#undef DEBUG_PRINTS

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

template<class Array1, class Array2>
void checkArraysEqual(const Array1& a, const Array2& b) {
    int errors = 0;
    for(int k=0; k<a.shape(2); ++k) {
    for(int j=0; j<a.shape(1); ++j) {
    for(int i=0; i<a.shape(0); ++i) {
        if( a(i,j,k) != b(i,j,k) ) {
            std::stringstream err;
            err << i << ", " << j << ", " << k << " : " << (int)a(i,j,k) << " vs. " << (int)b(i,j,k) << std::endl;
            std::cout << err.str();
            ++errors;
        }
        if(errors > 10) {
            throw std::runtime_error("too many errors");
        }
    }
    }
    }
}

int main() {
    vigra::HDF5File f("/home/tkroeger/datasets/snemi2013/test-input/test-input.h5", vigra::HDF5File::Open);
    vigra::MultiArray<5, vigra::UInt8> data;
    f.readAndResize("volume/data", data);

    auto data3 = data.bind<0>(0).bind<3>(0);

    std::cout << "building blocked array for data of shape = " << data3.shape() << std::endl;

    //vigra::MultiArrayShape<3>::type blockShape(37,66,87);
    //vigra::MultiArrayShape<3>::type blockShape(128,128,128);
    vigra::MultiArrayShape<3>::type blockShape(32,32,32);

    BlockedArray<3, unsigned char> dataBlocked(blockShape, data3);

    std::cout << data3.shape() << std::endl;

    typedef BlockedArray<3, unsigned char> B;

    std::vector<std::pair<B::difference_type, B::difference_type> > bounds;
    bounds.push_back(std::make_pair(B::difference_type(0,0,0), B::difference_type(100,100,100)));
    bounds.push_back(std::make_pair(B::difference_type(0,0,0), B::difference_type(100,200,100)));
    bounds.push_back(std::make_pair(B::difference_type(0,0,0), B::difference_type(100,128,128)));
    bounds.push_back(std::make_pair(B::difference_type(10,20,30), B::difference_type(90,140,150)));
    bounds.push_back(std::make_pair(B::difference_type(10,20,30), B::difference_type(90,800,666)));
    bounds.push_back(std::make_pair(B::difference_type(0,0,0), B::difference_type(100,200,300)));
    bounds.push_back(std::make_pair(B::difference_type(0,0,0), B::difference_type(100,1024,1024)));

    vigra::RandomMT19937 random;
    while(true) {
        B::difference_type p, q;
        for(int i=0; i<3; ++i) { p[i] = random.uniformInt(data3.shape(i)); }
        for(int i=0; i<3; ++i) { q[i] = p[i]+2+random.uniformInt(data3.shape(i)-p[i]-2); }

        bool ok = true;
        for(int i=0; i<3; ++i) {
            if(p[i] < 0 || p[i] >= data3.shape(i) || q[i] < 0 || q[i] > data3.shape(i)) {
                ok = false;
            }
            if(!(q[i] > p[i]+1)) {
                ok = false;
            }
        }
        if(ok) {
            auto x = random.uniformInt(3);
            q[x] = p[x]+1;

            bounds.push_back(std::make_pair(p,q));
        }
        if(bounds.size() == 1000) {
            break;
        }


    }

    std::cout << "*** UNIT TEST ***" << std::endl;
    using namespace std::chrono;
    
    for(const auto& pq : bounds) {
        const auto p = pq.first;
        const auto q = pq.second;
        std::cout << "* unit test read from " << p << " to " << q << " (" << std::sqrt((double)((q-p)[0]*(q-p)[1]*(q-p)[2])) << "^2" << ")" << std::endl;

        //read blocked
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        auto smallBlock = dataBlocked.readSubarray(p,q);
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> diff1 = duration_cast<duration<double>>(t2 - t1);

        CHECK_OP(smallBlock.shape(),==,data3.subarray(p, q).shape()," ");

        t1 = high_resolution_clock::now();
        vigra::MultiArrayView<3, vigra::UInt8> referenceBlock(data3.subarray(p,q));
        t2 = high_resolution_clock::now();
        duration<double> diff2 = duration_cast<duration<double>>(t2 - t1);
        std::cout << "  read blocked: " << diff1.count() << std::endl;

        checkArraysEqual(smallBlock, referenceBlock);
    }
    std::cout << "END: UNIT TEST" << std::endl;
    
    
    for(const auto& pq : bounds) {
        const auto p = pq.first;
        const auto q = pq.second;
        std::cout << "* unit test write from " << p << " to " << q << " (" << std::sqrt((double)((q-p)[0]*(q-p)[1]*(q-p)[2])) << "^2" << ")" << std::endl;
       
        vigra::MultiArray<3, vigra::UInt8> a(q-p);
        for(int i=0; i<a.size(); ++i) {
            a[i] = random.uniformInt(256);
        }
        
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        dataBlocked.writeSubarray(p,q, a);
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> diff1 = duration_cast<duration<double>>(t2 - t1);
        std::cout << "  write blocked: " << diff1.count() << " sec." << std::endl;
        
        data3.subarray(p,q) = a;
        
        checkArraysEqual(data3, dataBlocked.readSubarray({0,0,0}, {100,1024,1024}));
        
    }

    B::difference_type p(10,20,30);
    B::difference_type q(90,800,666);
    auto smallBlock = dataBlocked.readSubarray(p,q);
    auto img = smallBlock.bind<0>(smallBlock.shape(0)/2); 
    vigra::BImage image(img.shape(0), img.shape(1));
    for(int i=0; i<img.shape(0); ++i) {
    for(int j=0; j<img.shape(1); ++j) {
        image(i,j) = img(i,j);
    }
    }
    
    vigra::exportImage(srcImageRange(image), vigra::ImageExportInfo("/tmp/t.jpg").setCompression("90"));

    return 0;
}
