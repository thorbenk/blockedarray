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
    
typedef Array<N,T> BA;
typedef typename BA::V V;
typedef typename BA::BlockPtr BlockPtr;
typedef typename BA::BlockList BlockList;
typedef vigra::MultiArray<N,T> A;
   
static void testDirtySlicewise(
    int verbose = false
) {
    V sh(100,100);
    V blockShape(10,10);
    
    vigra::MultiArray<2,T> initialData(sh, 1);
    BA blockedArray(blockShape, initialData);
    
    blockedArray.setDirty(V(), sh, true); //set everything dirty
    blockedArray.setDirty(V(5,0), V(6,100), false); // x-slice 5 is not dirty anymore
    shouldEqual(blockedArray.dirtyBlocks(V(), sh).size(), 100); //all blocks still dirty
    
    should(!blockedArray.isDirty(V(5,0), V(6,100)));
    should(!blockedArray.isDirty(V(5,50), V(6,75)));
    
    should(blockedArray.isDirty(V(5,0), V(7,100)));
    should(blockedArray.isDirty(V(5,25), V(7,75)));
    
    blockedArray.setDirty(V(), V(10,10), false);
    shouldEqual(blockedArray.dirtyBlocks(V(), sh).size(), 99);
    
    blockedArray.setDirty(V(), V(10,10), true);
    shouldEqual(blockedArray.dirtyBlocks(V(), sh).size(), 100);
}

static void testDirty(
    int verbose = false
) {
    V sh(100,100);
    V blockShape(10,10);
    
    vigra::MultiArray<2,T> initialData(sh, 1);
    BA blockedArray(blockShape, initialData);
    
    BlockList bl = blockedArray.dirtyBlocks(V(), sh);
    shouldEqual(bl.size(), 0);
    
    blockedArray.setDirty(V(), sh, true);
    BlockList bl2 = blockedArray.dirtyBlocks(V(), sh);
    shouldEqual(bl2.size(), 10*10);
    
    blockedArray.deleteSubarray(V(), sh);
    shouldEqual(blockedArray.numBlocks(), 0);
   
    //partial write to block (0,0)
    blockedArray.writeSubarray(V(1,1), V(3,4), A(V(2,3), 77) ); //write 
    BlockList bl3 = blockedArray.dirtyBlocks(V(), sh);
    shouldEqual(bl3.size(), 1);
    shouldEqual(bl3[0], V(0,0));
   
    //partial write to block (0,1)
    blockedArray.writeSubarray(V(2,12), V(4,16), A(V(2,6), 99) ); //write 
    BlockList bl4 = blockedArray.dirtyBlocks(V(), sh);
    shouldEqual(bl4.size(), 2);
    shouldEqual(bl4[0], V(0,0));
    shouldEqual(bl4[1], V(0,1));
    
    //full write to block (0,0)
    blockedArray.writeSubarray(V(0,0), V(10,10), A(V(10,10), 11) ); //write 
    BlockList bl5 = blockedArray.dirtyBlocks(V(), sh);
    shouldEqual(bl5.size(), 1);
    shouldEqual(bl5[0], V(0,1));
}

static void testWriteSubarrayNonzero(
    int verbose = false
) {
    vigra::MultiArray<3,T> initialData(V(100,75,111), 1);
    BA blockedArray(V(20,30,40), initialData);
    
    vigra::MultiArray<3,T> w(V(4,4,4));
    w[V(0,1,0)] = 42;
    w[V(1,0,0)] = 100;
    
    blockedArray.writeSubarrayNonzero(V(), w.shape(), w, 100);
    
    vigra::MultiArray<3,T> r(initialData.shape());
    blockedArray.readSubarray(V(), r.shape(), r);
    shouldEqual(r[V(0,0,0)], 1);
    shouldEqual(r[V(0,1,0)], 42);
    shouldEqual(r[V(1,0,0)], 0);

    shouldEqual(blockedArray[V(0,0,0)], 1);
    shouldEqual(blockedArray[V(0,1,0)], 42);
    shouldEqual(blockedArray[V(1,0,0)], 0);

    V p(7,60,43);
    blockedArray.write(p, 55);
    shouldEqual(blockedArray[p], 55);
    blockedArray.write(p, 59);
    shouldEqual(blockedArray[p], 59);
}

static void testApplyRelabeling(
    int verbose = false
) {
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
    vigra::MultiArray<N,T> theData(dataShape);
    
    //If delete empty blocks is disabled, all-zero blocks remain
    //after overwriting the whole array with zeros
    {
        BA blockedArray(blockShape, theData);
        should(blockedArray.numBlocks() > 0);
        blockedArray.setDeleteEmptyBlocks(false);
        vigra::MultiArray<N,T> zeros(theData.shape());
        blockedArray.writeSubarray(V(), theData.shape(), zeros);
        should(blockedArray.numBlocks() > 0);
    }
    //OTOH, if delete empty blocks is emabled, all blocks are deleted
    //on overwriting the whole array with zeros
    {
        BA blockedArray(blockShape, theData);
        should(blockedArray.numBlocks() > 0);
        blockedArray.setDeleteEmptyBlocks(true);
        vigra::MultiArray<N,T> zeros(theData.shape());
        blockedArray.writeSubarray(V(), theData.shape(), zeros);
        shouldEqual(blockedArray.numBlocks(), 0);
    }
}

static void test(typename vigra::MultiArray<N,T>::difference_type dataShape,
          typename vigra::MultiArray<N,T>::difference_type blockShape,
          int nSamples = 100,
          int verbose = false
) {
    if(verbose) { std::cout << "TEST" << std::endl; }
    
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
    
    typedef std::vector<std::pair<V, V> > BoundsList;
    BoundsList bounds;
    
    vigra::RandomMT19937 random;
    while(bounds.size() < nSamples) {
        V p, q;
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
        const V p= pq.first;
        const V q = pq.second;
        
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
        const V p = pq.first;
        const V q = pq.second;
        
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
        blockedArray.readSubarray(V(), theData.shape(), r);
        
        should(arraysEqual(theData, r)); 
        ++n;
    }
    if(!verbose) { std::cout << std::endl; }
   
    //
    // delete array
    //
    blockedArray.deleteSubarray(V(), theData.shape());
    
    shouldEqual(blockedArray.numBlocks(),0);
}
}; /*struct ArrayTest*/

struct ArrayTestImpl {
    void dim2_testDirtySlicewise() {
        ArrayTest<2, vigra::UInt8>::testDirtySlicewise();
        std::cout << "... passed dim2_testDirtySlicewise" << std::endl;
    }
    void dim2_testDirty() {
        ArrayTest<2, vigra::UInt8>::testDirty();
        std::cout << "... passed dim2_testDirty" << std::endl;
    }
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
    void dim3_testWriteSubarrayNonzero() {
        ArrayTest<3, vigra::UInt32>::testWriteSubarrayNonzero(false);
        std::cout << "... passed dim3_testWriteSubarrayNonzero" << std::endl;
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
        add( testCase(&ArrayTestImpl::dim2_testDirtySlicewise));
        add( testCase(&ArrayTestImpl::dim2_testDirty));
        add( testCase(&ArrayTestImpl::dim3_testWriteSubarrayNonzero));
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
