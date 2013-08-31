#include <vector>
#include <iostream>
#include <map>
#include <cmath>

#include <vigra/multi_array.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/impex.hxx>
#include <vigra/timing.hxx>
#include <vigra/unittest.hxx>

#define DEBUG_CHECKS 1
#undef DEBUG_PRINTS

#include <bw/array.h>

#include "test_utils.h"

#include <bw/extern_templates.h>

using namespace BW;

template<int N, class T>
struct ArrayTest {

static void testApplyRelabeling(
    int verbose = false
) {
    typedef Array<3,T> BA;
    typedef typename BA::V V;
   
    vigra::MultiArray<3,T> initialData(V(100,200,300));
    initialData[V(3,4,5)]     = 2;
    initialData[V(80,99,260)] = 3;
    BA blockedArray(V(20,30,40), initialData);
    
    vigra::MultiArray<1, T> relabeling(vigra::Shape1(5)); 
    relabeling[2] = 42;
    relabeling[3] = 99;
    blockedArray.applyRelabeling(relabeling);
    
    initialData[V(3,4,5)]     = 42;
    initialData[V(80,99,260)] = 99;
    
    vigra::MultiArray<3,T> read(V(100,200,300));
    blockedArray.readSubarray(V(), V(100,200,300), read);
    
    shouldEqualSequence(initialData.begin(), initialData.end(), read.begin());
}
    
static void testManageCoordinateLists(
    int verbose = false
) {
    typedef Array<3,T> BA;
    typedef typename BA::V V;
   
    vigra::MultiArray<3,T> initialData(V(100,200,300));
    initialData[V(3,4,5)]     = 2;
    initialData[V(80,99,260)] = 3;
    
    BA blockedArray(V(20,30,40), initialData);
    blockedArray.setManageCoordinateLists(true);
    
    typename BA::VoxelValues vv = blockedArray.nonzero();
    shouldEqual(vv.first[0], V(3,4,5));
    shouldEqual(vv.second[0], 2);
    shouldEqual(vv.first[1], V(80,99,260));
    shouldEqual(vv.second[1], 3);
    
    blockedArray.setManageCoordinateLists(false);
    shouldEqual(blockedArray.nonzero().first.size(), 0);
    
    blockedArray.setManageCoordinateLists(true);
    typename BA::VoxelValues vv2 = blockedArray.nonzero();
    shouldEqual(vv2.first[0], V(3,4,5));
    shouldEqual(vv2.second[0], 2);
    shouldEqual(vv2.first[1], V(80,99,260));
    shouldEqual(vv2.second[1], 3);
    
    vigra::MultiArray<3,T> toWrite(V(2,2,2));
    toWrite(0,1,1) = 42;
    blockedArray.writeSubarray(V(1,1,1), V(3,3,3), toWrite);
    typename BA::VoxelValues vv3 = blockedArray.nonzero();
    shouldEqual(vv3.first[0], V(1,2,2));
    shouldEqual(vv3.second[0], 42);
    shouldEqual(vv3.first[1], V(3,4,5));
    shouldEqual(vv3.second[1], 2);
    shouldEqual(vv3.first[2], V(80,99,260));
    shouldEqual(vv3.second[2], 3);
}
    
static void testMinMax(
    int verbose = false
) {
    typedef Array<3,T> BA;
    typedef typename BA::V V;
   
    vigra::MultiArray<3,T> zeros(V(100,200,300));
    
    BA blockedArray(V(20,30,40), zeros);
    
    std::pair<T,T> mm = blockedArray.minMax();
    shouldEqual(mm.first, std::numeric_limits<T>::max());
    shouldEqual(mm.second, std::numeric_limits<T>::min());
    
    blockedArray.setMinMaxTrackingEnabled(true);
    mm = blockedArray.minMax();
    shouldEqual(mm.first, 0);
    shouldEqual(mm.second, 0);
   
    vigra::MultiArray<3,T> ones(V(3,3,2), 1);
    blockedArray.writeSubarray(V(2,3,4), V(5,6,6), ones);
    
    mm = blockedArray.minMax();
    shouldEqual(mm.first, 0);
    shouldEqual(mm.second, 1);
    
    vigra::MultiArray<3,T> zeros2(V(3,3,2));
    blockedArray.writeSubarray(V(2,3,4), V(5,6,6), zeros);
    
    mm = blockedArray.minMax();
    shouldEqual(mm.first, 0);
    shouldEqual(mm.second, 0);
}
    
static void testCompression(
    typename vigra::MultiArray<N,T>::difference_type dataShape,
    typename vigra::MultiArray<N,T>::difference_type blockShape,
    int verbose = false
) {
    
    typedef Array<N,T> BA;
    typedef typename BA::V diff_t;
    typedef typename BA::BlockPtr BlockPtr;
    
    vigra::MultiArray<N,T> theData(dataShape);
    FillRandom<T, typename vigra::MultiArray<N,T>::iterator>::fillRandom(theData.begin(), theData.end());
    
    if(verbose) std::cout << "* constructing Array with blockShape=" << blockShape << ", dataShape=" << theData.shape() << std::endl;
    
    BA blockedArray(blockShape, theData);
    
    shouldEqualTolerance(blockedArray.averageCompressionRatio(), 1.0, 1E-10);
    should(blockedArray.numBlocks() > 0);
    should(blockedArray.sizeBytes() > 0);
    
    BOOST_FOREACH(const typename BA::BlocksMap::value_type& b, blockedArray.blocks_) {
        should(!b.second->isCompressed());
    }
    
    blockedArray.setCompressionEnabled(true);
    
    BOOST_FOREACH(const typename BA::BlocksMap::value_type& b, blockedArray.blocks_) {
        should(b.second->isCompressed());
    }
    should(blockedArray.averageCompressionRatio() < 0.9);
    
    blockedArray.setCompressionEnabled(false);
    
    BOOST_FOREACH(const typename BA::BlocksMap::value_type& b, blockedArray.blocks_) {
        should(!b.second->isCompressed());
    }
    shouldEqualTolerance(blockedArray.averageCompressionRatio(), 1.0, 1E-10);
}

static void testDeleteEmptyBlocks(
    typename vigra::MultiArray<N,T>::difference_type dataShape,
    typename vigra::MultiArray<N,T>::difference_type blockShape,
    int verbose = false
) {
    typedef Array<N,T> BA;
    typedef typename BA::V diff_t;
    typedef typename BA::BlockPtr BlockPtr;
    vigra::MultiArray<N,T> theData(dataShape);
    
    //If delete empty blocks is disabled, all-zero blocks remain
    //after overwriting the whole array with zeros
    {
        BA blockedArray(blockShape, theData);
        should(blockedArray.numBlocks() > 0);
        blockedArray.setDeleteEmptyBlocks(false);
        vigra::MultiArray<N,T> zeros(theData.shape());
        blockedArray.writeSubarray(diff_t(), theData.shape(), zeros);
        should(blockedArray.numBlocks() > 0);
    }
    //OTOH, if delete empty blocks is emabled, all blocks are deleted
    //on overwriting the whole array with zeros
    {
        BA blockedArray(blockShape, theData);
        should(blockedArray.numBlocks() > 0);
        blockedArray.setDeleteEmptyBlocks(true);
        vigra::MultiArray<N,T> zeros(theData.shape());
        blockedArray.writeSubarray(diff_t(), theData.shape(), zeros);
        shouldEqual(blockedArray.numBlocks(), 0);
    }
}

static void test(typename vigra::MultiArray<N,T>::difference_type dataShape,
          typename vigra::MultiArray<N,T>::difference_type blockShape,
          int nSamples = 100,
          int verbose = false
) {
    if(verbose) { std::cout << "TEST" << std::endl; }
    typedef Array<N,T> BA;
    typedef typename BA::V diff_t;
    
    vigra::MultiArray<N,T> theData(dataShape);
    FillRandom<T, typename vigra::MultiArray<N,T>::iterator>::fillRandom(theData.begin(), theData.end());
    
    if(verbose) std::cout << "* constructing Array with blockShape=" << blockShape << ", dataShape=" << theData.shape() << std::endl;
    
    {
        BA emptyArray(blockShape);
        shouldEqual(emptyArray.numBlocks(), 0);
        shouldEqual(emptyArray.sizeBytes(), 0);
    }
    
    BA blockedArray(blockShape, theData);
    shouldEqualTolerance(blockedArray.averageCompressionRatio(), 1.0, 1E-10);
    should(blockedArray.numBlocks() > 0);
    should(blockedArray.sizeBytes() > 0);
    
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

        USETICTOC
        //read blocked
        TIC
        vigra::MultiArray<N, T> smallBlock(q-p);
        blockedArray.readSubarray(p,q, smallBlock);
        if(verbose) std::cout << "  read blocked: " << TOCS << std::endl;

        shouldEqual(smallBlock.shape(),theData.subarray(p, q).shape());

        vigra::MultiArray<N, T> referenceBlock(theData.subarray(p,q));

        should(arraysEqual(smallBlock, referenceBlock));
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
        
        USETICTOC
        TIC
        blockedArray.writeSubarray(p,q, a);
        if(verbose) std::cout << "  write blocked: " << TOCS << std::endl;
        
        theData.subarray(p,q) = a;
        
        vigra::MultiArray<N, T> r(theData.shape());
        blockedArray.readSubarray(diff_t(), theData.shape(), r);
        
        should(arraysEqual(theData, r)); 
        ++n;
    }
    if(!verbose) { std::cout << std::endl; }
   
    //
    // delete array
    //
    blockedArray.deleteSubarray(diff_t(), theData.shape());
    
    shouldEqual(blockedArray.numBlocks(),0);
}
}; /*struct ArrayTest*/

struct ArrayTestImpl {
    void dim3_uint8() {
        ArrayTest<3, vigra::UInt8>::test(vigra::Shape3(89,66,77), vigra::Shape3(22,11,9), 50);
        ArrayTest<3, vigra::UInt8>::test(vigra::Shape3(100,88,50), vigra::Shape3(13,23,7), 50);
        std::cout << "... passed dim3_uint8" << std::endl;
    }
    void dim3_float32() {
        ArrayTest<3, float>::test(vigra::Shape3(75,89,111), vigra::Shape3(22,11,15), 50);
        std::cout << "... passed dim3_float32" << std::endl;
    }
    void dim5_float32() {
        ArrayTest<5, float>::test(vigra::Shape5(1,75,100,200,1), vigra::Shape5(1,22,11,15,1), 50);
        ArrayTest<5, float>::test(vigra::Shape5(3,75,33,67,1), vigra::Shape5(2,22,11,15,1), 50);
        std::cout << "... passed dim5_float32" << std::endl;
    }
    void dim3_testCompression() {
        ArrayTest<3, vigra::UInt32>::testCompression(vigra::Shape3(100,88,50), vigra::Shape3(13,23,7), false);
        std::cout << "... passed dim3_testCompression" << std::endl;
    }
    void dim3_testDeleteEmptyBlocks() {
        ArrayTest<3, vigra::UInt32>::testDeleteEmptyBlocks(vigra::Shape3(100,88,50), vigra::Shape3(13,23,7), false);
        std::cout << "... passed dim3_testDeleteEmptyBlocks" << std::endl;
    }
    void dim3_testMinMax() {
        ArrayTest<3, vigra::UInt32>::testMinMax(false);
        std::cout << "... passed dim3_testMinMax" << std::endl;
    }
    void dim3_testApplyRelabeling() {
        ArrayTest<3, vigra::UInt32>::testApplyRelabeling(false);
        std::cout << "... passed dim3_testApplyRelabeling" << std::endl;
    }
    void dim3_testManageCoordinateLists() {
        ArrayTest<3, vigra::UInt32>::testManageCoordinateLists(false);
        std::cout << "... passed dim3_testManageCoordinateLists" << std::endl;
    }
}; /* struct ArrayTest */

struct ArrayTestSuite : public vigra::test_suite {
    ArrayTestSuite()
        : vigra::test_suite("ArrayTestSuite")
    {
        add( testCase(&ArrayTestImpl::dim3_testApplyRelabeling));
        add( testCase(&ArrayTestImpl::dim3_testManageCoordinateLists));
        add( testCase(&ArrayTestImpl::dim3_testMinMax));
        add( testCase(&ArrayTestImpl::dim3_testDeleteEmptyBlocks));
        add( testCase(&ArrayTestImpl::dim3_testCompression));
        add( testCase(&ArrayTestImpl::dim3_uint8));
        add( testCase(&ArrayTestImpl::dim3_float32));
        add( testCase(&ArrayTestImpl::dim5_float32));
    }
};

int main(int argc, char ** argv) {
    ArrayTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
