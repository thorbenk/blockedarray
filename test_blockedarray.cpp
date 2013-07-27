#include <vector>
#include <iostream>
#include <map>
#include <cmath>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestCompressedArray
#include <boost/test/unit_test.hpp>

#include <boost/timer/timer.hpp>

#include <vigra/multi_array.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>

#define DEBUG_CHECKS 1
#undef DEBUG_PRINTS

#include "blockedarray.h"
#include "test_utils.h"

//TODO: use alignas to allocate char arrays

template<int N, class T>
void test(typename vigra::MultiArray<N,T>::difference_type dataShape,
          typename vigra::MultiArray<N,T>::difference_type blockShape,
          int nSamples = 100,
          int verbose = false
) {
    if(verbose) { std::cout << "TEST" << std::endl; }
    typedef BlockedArray<N,T> BA;
    typedef typename BA::difference_type diff_t;
    
    vigra::MultiArray<N,T> theData(dataShape);
    FillRandom<T, typename vigra::MultiArray<N,T>::iterator>::fillRandom(theData.begin(), theData.end());
    
    if(verbose) std::cout << "* constructing BlockedArray with blockShape=" << blockShape << ", dataShape=" << theData.shape() << std::endl;
    
    {
        BA emptyArray(blockShape);
        BOOST_CHECK_EQUAL(emptyArray.numBlocks(), 0);
        BOOST_CHECK_EQUAL(emptyArray.sizeBytes(), 0);
    }
    
    BA blockedArray(blockShape, theData);
    BOOST_CHECK_CLOSE(blockedArray.averageCompressionRatio(), 0.0, 1E-10);
    BOOST_CHECK(blockedArray.numBlocks() > 0);
    BOOST_CHECK(blockedArray.sizeBytes() > 0);
    
    typedef std::vector<std::pair<diff_t, diff_t> > BoundsList;
    BoundsList bounds;
    
    vigra::RandomMT19937 random;
    while(bounds.size() < nSamples) {
        diff_t p, q;
        for(int i=0; i<N; ++i) { p[i] = random.uniformInt(theData.shape(i)); }
        for(int i=0; i<N; ++i) { q[i] = p[i]+1+random.uniformInt(theData.shape(i)-p[i]); }
        
        bool ok = true;
        for(int i=0; i<N; ++i) {
            if(p[i] < 0 || p[i] >= theData.shape(i) || q[i] < 0 || q[i] > theData.shape(i)) {
                ok = false;
            }
            if(!(q[i] >= p[i]+1)) {
                ok = false;
            }
        }
        if(ok) {
            int x = random.uniformInt(N);
            q[x] = p[x]+1;

            bounds.push_back(std::make_pair(p,q));
        }
    }
    
    //
    // test read access of ROI
    //
    int n = 0;
    BOOST_FOREACH(typename BoundsList::value_type pq, bounds) {
        const diff_t p= pq.first;
        const diff_t q = pq.second;
        
        if(verbose) {
            std::cout << "* unit test read from " << p << " to " << q << " (" << std::sqrt((double)((q-p)[0]*(q-p)[1]*(q-p)[2])) << "^2" << ")" << std::endl;
        }
        else {
            std::cout << "read: " << n << " of " << nSamples << "             \r" << std::flush;
        }

        //read blocked
        boost::timer::cpu_timer t1;
        vigra::MultiArray<N, T> smallBlock(q-p);
        blockedArray.readSubarray(p,q, smallBlock);
        if(verbose) std::cout << "  read blocked: " << t1.format(10, "%w sec.") << std::endl;

        BOOST_CHECK_EQUAL(smallBlock.shape(),theData.subarray(p, q).shape());

        vigra::MultiArray<N, T> referenceBlock(theData.subarray(p,q));

        BOOST_CHECK(arraysEqual(smallBlock, referenceBlock));
        ++n;
    }
    if(!verbose) { std::cout << std::endl; }
    
    //
    // test write access of ROI
    //
    n = 0;
    BOOST_FOREACH(typename BoundsList::value_type pq, bounds) {
        const diff_t p = pq.first;
        const diff_t q = pq.second;
        
        if(verbose) {
            std::cout << "* unit test write from " << p << " to " << q << " (" << std::sqrt((double)((q-p)[0]*(q-p)[1]*(q-p)[2])) << "^2" << ")" << std::endl;
        }
        else {
            std::cout << "write: " << n << " of " << nSamples << "             \r" << std::flush;
        }
       
        vigra::MultiArray<N, T> a(q-p);
        FillRandom<T, typename vigra::MultiArray<N,T>::iterator>::fillRandom(a.begin(), a.end());
        
        //for(int i=0; i<a.size(); ++i) {
        //    a[i] = random.uniformInt(256);
        //}
        
        boost::timer::cpu_timer t1;
        blockedArray.writeSubarray(p,q, a);
        if(verbose) std::cout << "  write blocked: " << t1.format(10, "%w sec.") << std::endl;
        
        theData.subarray(p,q) = a;
        
        vigra::MultiArray<N, T> r(theData.shape());
        blockedArray.readSubarray(diff_t(), theData.shape(), r);
        
        BOOST_CHECK(arraysEqual(theData, r)); 
        ++n;
    }
    if(!verbose) { std::cout << std::endl; }
   
    //
    // delete array
    //
    blockedArray.deleteSubarray(diff_t(), theData.shape());
    
    BOOST_CHECK_EQUAL(blockedArray.numBlocks(),0);
}

BOOST_AUTO_TEST_CASE( dim3_uint8 ) {   
    test<3, vigra::UInt8>(vigra::Shape3(89,166,123), vigra::Shape3(22,33,44), 50);
}
BOOST_AUTO_TEST_CASE( dim3_float32 ) {   
    test<3, float>(vigra::Shape3(75,100,200), vigra::Shape3(22,11,15), 50);
}
BOOST_AUTO_TEST_CASE( dim5_float32 ) {   
    test<5, float>(vigra::Shape5(1,75,100,200,1), vigra::Shape5(1,22,11,15,1), 50);
    test<5, float>(vigra::Shape5(3,75,33,67,1), vigra::Shape5(2,22,11,15,1), 50);
}
BOOST_AUTO_TEST_CASE( dim3_int64 ) {   
    test<3, vigra::Int64>(vigra::Shape3(200,300,100), vigra::Shape3(22,33,44), 50);
}
