#include <vector>
#include <iostream>
#include <map>
#include <cmath>

#include <boost/timer/timer.hpp>

#include <vigra/multi_array.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>
#include <vigra/random.hxx>

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

#define DEBUG_CHECKS 1
#undef DEBUG_PRINTS

#include "blockedarray.h"

//TODO: use alignas to allocate char arrays

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

    vigra::MultiArrayView<3, vigra::UInt8> data3 = data.bind<0>(0).bind<3>(0);

    std::cout << "building blocked array for data of shape = " << data3.shape() << std::endl;
    //vigra::MultiArrayShape<3>::type blockShape(37,66,87);
    //vigra::MultiArrayShape<3>::type blockShape(128,128,128);
    vigra::MultiArrayShape<3>::type blockShape(32,32,32);
    BlockedArray<3, unsigned char> dataBlocked(blockShape, data3);

    std::cout << "average compression ratio: " << dataBlocked.averageCompressionRatio() << std::endl;
    std::cout << "current size: "
              << static_cast<double>(dataBlocked.sizeBytes())/(1024.0*1024.0)
              << " MB" << std::endl;

    typedef BlockedArray<3, unsigned char> B;

    typedef std::vector<std::pair<B::difference_type, B::difference_type> > BoundsList;
    BoundsList bounds;
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
            int x = random.uniformInt(3);
            q[x] = p[x]+1;

            bounds.push_back(std::make_pair(p,q));
        }
        if(bounds.size() == 1000) {
            break;
        }


    }

    std::cout << "*** UNIT TEST ***" << std::endl;
   
    B::difference_type outsideP(4000,5000,6000);
    B::difference_type outsideQ(4200,5020,6030);
    
    //read outside of dataset
    //should return array full of zeros
    
    vigra::MultiArray<3, vigra::UInt8> outside(outsideQ-outsideP);
    dataBlocked.readSubarray(outsideP, outsideQ, outside);
   
    std::fill(outside.begin(), outside.end(), 42);
    dataBlocked.writeSubarray(outsideP, outsideQ, outside);
    
    //CHECK_OP(dataBlocked.readSubarray(outsideP, outsideQ)(0,0,0),==,42," ");
    
    BOOST_FOREACH(BoundsList::value_type pq, bounds) {
        const B::difference_type p = pq.first;
        const B::difference_type q = pq.second;
        std::cout << "* unit test read from " << p << " to " << q << " (" << std::sqrt((double)((q-p)[0]*(q-p)[1]*(q-p)[2])) << "^2" << ")" << std::endl;

        //read blocked
        boost::timer::cpu_timer t1;
        vigra::MultiArray<3, vigra::UInt8> smallBlock(q-p);
        dataBlocked.readSubarray(p,q, smallBlock);
        std::cout << "  read blocked: " << t1.format(10, "%w sec.") << std::endl;

        CHECK_OP(smallBlock.shape(),==,data3.subarray(p, q).shape()," ");

        vigra::MultiArrayView<3, vigra::UInt8> referenceBlock(data3.subarray(p,q));

        checkArraysEqual(smallBlock, referenceBlock);
    }
    std::cout << "END: UNIT TEST" << std::endl;
    
    
    BOOST_FOREACH(BoundsList::value_type pq, bounds) {
        const B::difference_type p = pq.first;
        const B::difference_type q = pq.second;
        std::cout << "* unit test write from " << p << " to " << q << " (" << std::sqrt((double)((q-p)[0]*(q-p)[1]*(q-p)[2])) << "^2" << ")" << std::endl;
       
        vigra::MultiArray<3, vigra::UInt8> a(q-p);
        for(int i=0; i<a.size(); ++i) {
            a[i] = random.uniformInt(256);
        }
        
        boost::timer::cpu_timer t1;
        dataBlocked.writeSubarray(p,q, a);
        std::cout << "  write blocked: " << t1.format(10, "%w sec.") << std::endl;
        
        data3.subarray(p,q) = a;
        
        vigra::MultiArray<3, vigra::UInt8> r(data3.shape());
        dataBlocked.readSubarray(B::difference_type(), data3.shape(), r);
        checkArraysEqual(data3, r); 
        
    }

    B::difference_type p(10,20,30);
    B::difference_type q(90,800,666);
    vigra::MultiArray<3, vigra::UInt8> smallBlock(q-p);
    dataBlocked.readSubarray(p,q, smallBlock);
    vigra::MultiArrayView<2, vigra::UInt8> img = smallBlock.bind<0>(smallBlock.shape(0)/2); 
    vigra::BImage image(img.shape(0), img.shape(1));
    for(int i=0; i<img.shape(0); ++i) {
    for(int j=0; j<img.shape(1); ++j) {
        image(i,j) = img(i,j);
    }
    }
    
    vigra::exportImage(srcImageRange(image), vigra::ImageExportInfo("/tmp/t.jpg").setCompression("90"));

    return 0;
}
