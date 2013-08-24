#include <iostream>

#include <bw/compressedarray.h>

#include "test_utils.h"

#include <vigra/unittest.hxx>

using namespace BW;

template<int N, class T>
void testCompressedArray(typename vigra::MultiArray<N,T>::difference_type dataShape)
{
    vigra::MultiArray<N,T> theData(dataShape);
    FillRandom<T, typename vigra::MultiArray<N,T>::iterator>::fillRandom(theData.begin(), theData.end());
    
    typedef CompressedArray<N, T> CA;
    
    {
        CA e; //empty
        should(!e.isCompressed());
        shouldEqual(e.compressedSize(), 0);
        should(!e.isDirty());
    }

    //std::cout << "construct" << std::endl;
    CA ca(theData);
    should(!ca.isDirty());
    should(!ca.isCompressed());
    shouldEqual(ca.compressedSize(), 0);
    shouldEqual(ca.shape(), theData.shape());
    
    //std::cout << "read" << std::endl;
    vigra::MultiArray<N,T> r(dataShape);
    ca.readArray(r);
    should(arraysEqual(theData, r)); 
    
    //std::cout << "compress & uncompress" << std::endl;
    ca.compress();
    std::fill(r.begin(), r.end(), 0);
    ca.readArray(r);
    should(arraysEqual(theData, r)); 
    
    should(ca.isCompressed());
    should(ca.compressedSize() > 0);
    ca.uncompress();
    should(ca.compressedSize() > 0);
    should(!ca.isCompressed());
    std::fill(r.begin(), r.end(), 0);
    ca.readArray(r);
    should(arraysEqual(theData, r)); 

    //std::cout << "compress & read" << std::endl;
    ca.compress();
    should(ca.isCompressed());
    ca.readArray(r);
    should(arraysEqual(theData, r)); 
    
    //std::cout << "copy-construct" << std::endl;
    {
        CA ca2(ca);
        vigra::MultiArray<N,T> r1(dataShape);
        vigra::MultiArray<N,T> r2(dataShape);
        ca.readArray(r1);
        ca2.readArray(r2);
        should(arraysEqual(r1, r2));
    }
   
    //std::cout << "assignment" << std::endl;
    {
        CA ca2;
        ca2 = ca;
        vigra::MultiArray<N,T> r1(dataShape);
        vigra::MultiArray<N,T> r2(dataShape);
        ca.readArray(r1);
        ca2.readArray(r2);
        should(arraysEqual(r1, r2));
    }
   
    should(!ca.isDirty());
    ca.setDirty(true);
    should(ca.isDirty());
  
    {
        vigra::MultiArray<N,T> toWrite(ca.shape());
        std::fill(toWrite.begin(), toWrite.end(), 42);
        ca.writeArray(typename CA::difference_type(), ca.shape(), toWrite);
        should(!ca.isDirty());
        std::fill(r.begin(), r.end(), 0);
        ca.readArray(r);
        should(arraysEqual(r, toWrite));
       
        ca.setDirty(true);
        //now, write only a subset of the data.
        //the whole block has to stay dirty
        std::fill(toWrite.begin(), toWrite.end(), 41);
        typename CA::difference_type end;
        std::fill(end.begin(), end.end(), 1);
        should(ca.shape()[0] > 1);
        ca.writeArray(typename CA::difference_type(), end, toWrite.subarray(typename CA::difference_type(), end));
        should(ca.isDirty());
    }
    
    ca.compress();
    
    {
        vigra::MultiArray<N,T> toWrite(ca.shape());
        std::fill(toWrite.begin(), toWrite.end(), 42);
        ca.writeArray(typename CA::difference_type(), ca.shape(), toWrite);
        std::fill(r.begin(), r.end(), 0);
        ca.readArray(r);
        should(arraysEqual(r, toWrite));
    }
}

struct CompressedArrayTest {
void dirtyness3() {
    typedef vigra::MultiArray<3, int> Array;
    using vigra::Shape3;
   
    Array data(Shape3(10,30,40));
    std::fill(data.begin(), data.end(), 42);
   
    CompressedArray<3, int> ca(data);
 
    ca.setDirty(true);
    should(ca.isDirty(0,0));
    should(ca.isDirty(0,3));
    should(ca.isDirty(1,0));
    should(ca.isDirty(1,3));
    should(ca.isDirty(2,0));
    should(ca.isDirty(2,3));
    ca.setDirty(false);
    should(!ca.isDirty(0,0));
    should(!ca.isDirty(0,3));
    should(!ca.isDirty(1,0));
    should(!ca.isDirty(1,3));
    should(!ca.isDirty(2,0));
    should(!ca.isDirty(2,3));
    ca.setDirty(3,7, true);
    should(ca.isDirty(3,7));
    ca.setDirty(3,7, false);
    should(!ca.isDirty(3,7));
    
    ca.setDirty(true);
    
    {
        Shape3 p(4,0,0), q(5,30,40); //write a 2D slice
        ca.writeArray(p,q, data.subarray(p,q));
        should(ca.isDirty());
        should(!ca.isDirty(0, 4));
    }
  
    {
        Shape3 p(0,6,0), q(10,7,40); //write a 2D slice
        ca.writeArray(p,q, data.subarray(p,q));
        should(ca.isDirty());
        should(!ca.isDirty(1, 6));
    }
    
    for(int i=0; i<10; ++i) {
        Shape3 p(i,0,0), q(i+1,30,40); //write a 2D slice
        ca.writeArray(p,q, data.subarray(p,q));
        should(!ca.isDirty(0, i));
    }
    should(!ca.isDirty());
}

void testDim1() {
    testCompressedArray<1, vigra::UInt8 >(vigra::Shape1(20));
    testCompressedArray<1, vigra::UInt16>(vigra::Shape1(21));
    testCompressedArray<1, vigra::UInt32>(vigra::Shape1(22));
    testCompressedArray<1, vigra::UInt64>(vigra::Shape1(23));
    testCompressedArray<1, vigra::Int32 >(vigra::Shape1(24));
    testCompressedArray<1, float        >(vigra::Shape1(25));
    testCompressedArray<1, vigra::Int64 >(vigra::Shape1(26));
}
    
void testDim2() {
    testCompressedArray<1, vigra::UInt8 >(vigra::Shape1(20));
    testCompressedArray<2, vigra::UInt8 >(vigra::Shape2(20,30));
    testCompressedArray<2, vigra::UInt32>(vigra::Shape2(21,31));
    testCompressedArray<2, float        >(vigra::Shape2(25,32));
    testCompressedArray<2, vigra::Int64 >(vigra::Shape2(22,33));
}
    
void testDim3() {
    testCompressedArray<1, vigra::UInt8 >(vigra::Shape1(20));
    testCompressedArray<3, vigra::UInt8 >(vigra::Shape3(24,31,45));
    testCompressedArray<3, vigra::UInt32>(vigra::Shape3(25,32,44));
    testCompressedArray<3, float        >(vigra::Shape3(26,34,43));
    testCompressedArray<3, vigra::Int64 >(vigra::Shape3(27,38,41));
}
    
void testDim5() {
    testCompressedArray<5, vigra::UInt8 >(vigra::Shape5(2,20,30,4,1));
    testCompressedArray<5, vigra::UInt32>(vigra::Shape5(2,18,35,3,1));
    testCompressedArray<5, float        >(vigra::Shape5(2,23,31,2,1));
    testCompressedArray<5, vigra::Int64 >(vigra::Shape5(2,15,30,5,1));
}
}; /* struct CompressedArrayTest */

struct CompressedArrayTestSuite : public vigra::test_suite {
    CompressedArrayTestSuite()
        : vigra::test_suite("BlockedArrayTestSuite")
    {
        add( testCase(&CompressedArrayTest::dirtyness3));
        add( testCase(&CompressedArrayTest::testDim1));
        add( testCase(&CompressedArrayTest::testDim2));
        add( testCase(&CompressedArrayTest::testDim3));
        add( testCase(&CompressedArrayTest::testDim5));
    }
};

int main(int argc, char ** argv) {
    CompressedArrayTestSuite test;
    int failed = test.run(vigra::testsToBeExecuted(argc, argv));
    std::cout << test.report() << std::endl;
    return (failed != 0);
}
