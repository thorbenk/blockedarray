#include <iostream>

#include "compressedarray.h"
#include "test_utils.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestCompressedArray
#include <boost/test/unit_test.hpp>

template<int N, class T>
void testCompressedArray(typename vigra::MultiArray<N,T>::difference_type dataShape)
{
    vigra::MultiArray<N,T> theData(dataShape);
    FillRandom<T, typename vigra::MultiArray<N,T>::iterator>::fillRandom(theData.begin(), theData.end());
    
    typedef CompressedArray<N, T> CA;

    //std::cout << "read" << std::endl;
    //read
    CA ca(theData);
    BOOST_CHECK(!ca.isDirty());
    BOOST_CHECK(!ca.isCompressed());
    BOOST_CHECK_EQUAL(ca.compressedSize(), 0);
    BOOST_CHECK_EQUAL(ca.shape(), theData.shape());
    vigra::MultiArray<N,T> r(dataShape);
    ca.readArray(r);
    BOOST_CHECK(arraysEqual(theData, r)); 
    
    //std::cout << "compress & uncompress" << std::endl;
    //compress & uncompress
    ca.compress();
    std::fill(r.begin(), r.end(), 0);
    ca.readArray(r);
    BOOST_CHECK(arraysEqual(theData, r)); 
    
    BOOST_CHECK(ca.isCompressed());
    BOOST_CHECK(ca.compressedSize() > 0);
    ca.uncompress();
    BOOST_CHECK(ca.compressedSize() > 0);
    BOOST_CHECK(!ca.isCompressed());
    std::fill(r.begin(), r.end(), 0);
    ca.readArray(r);
    BOOST_CHECK(arraysEqual(theData, r)); 

    //std::cout << "compress & read" << std::endl;
    //compress and read
    ca.compress();
    BOOST_CHECK(ca.isCompressed());
    ca.readArray(r);
    BOOST_CHECK(arraysEqual(theData, r)); 
    
    //std::cout << "copy-construct" << std::endl;
    //copy-construct
    {
        CA ca2(ca);
        vigra::MultiArray<N,T> r1(dataShape);
        vigra::MultiArray<N,T> r2(dataShape);
        ca.readArray(r1);
        ca2.readArray(r2);
        BOOST_CHECK(arraysEqual(r1, r2));
    }
   
    //std::cout << "assignment" << std::endl;
    //assignment
    {
        CA ca2;
        ca2 = ca;
        vigra::MultiArray<N,T> r1(dataShape);
        vigra::MultiArray<N,T> r2(dataShape);
        ca.readArray(r1);
        ca2.readArray(r2);
        BOOST_CHECK(arraysEqual(r1, r2));
    }
   
    BOOST_CHECK(!ca.isDirty());
    ca.setDirty(true);
    BOOST_CHECK(ca.isDirty());
  
    {
        vigra::MultiArray<N,T> toWrite(ca.shape());
        std::fill(toWrite.begin(), toWrite.end(), 42);
        ca.writeArray(typename CA::difference_type(), ca.shape(), toWrite);
        std::fill(r.begin(), r.end(), 0);
        ca.readArray(r);
        BOOST_CHECK(arraysEqual(r, toWrite));
    }
    
    ca.compress();
    
    {
        vigra::MultiArray<N,T> toWrite(ca.shape());
        std::fill(toWrite.begin(), toWrite.end(), 42);
        ca.writeArray(typename CA::difference_type(), ca.shape(), toWrite);
        std::fill(r.begin(), r.end(), 0);
        ca.readArray(r);
        BOOST_CHECK(arraysEqual(r, toWrite));
    }
}

BOOST_AUTO_TEST_CASE( testDim1 ) {   
    testCompressedArray<1, vigra::UInt8 >(vigra::Shape1(20));
    testCompressedArray<1, vigra::UInt16>(vigra::Shape1(20));
    testCompressedArray<1, vigra::UInt32>(vigra::Shape1(20));
    testCompressedArray<1, vigra::UInt64>(vigra::Shape1(20));
    testCompressedArray<1, vigra::Int32 >(vigra::Shape1(20));
    testCompressedArray<1, float        >(vigra::Shape1(20));
    testCompressedArray<1, vigra::Int64 >(vigra::Shape1(20));
}
    
BOOST_AUTO_TEST_CASE( testDim2 ) {   
    testCompressedArray<2, vigra::UInt8 >(vigra::Shape2(20,30));
    testCompressedArray<2, vigra::UInt32>(vigra::Shape2(20,30));
    testCompressedArray<2, float        >(vigra::Shape2(20,30));
    testCompressedArray<2, vigra::Int64 >(vigra::Shape2(20,30));
}
    
BOOST_AUTO_TEST_CASE( testDim3 ) {   
    testCompressedArray<3, vigra::UInt8 >(vigra::Shape3(20,30,40));
    testCompressedArray<3, vigra::UInt32>(vigra::Shape3(20,30,40));
    testCompressedArray<3, float        >(vigra::Shape3(20,30,40));
    testCompressedArray<3, vigra::Int64 >(vigra::Shape3(20,30,40));
}
    
BOOST_AUTO_TEST_CASE( testDim5 ) {   
    testCompressedArray<5, vigra::UInt8 >(vigra::Shape5(2,20,30,4,1));
    testCompressedArray<5, vigra::UInt32>(vigra::Shape5(2,20,30,4,1));
    testCompressedArray<5, float        >(vigra::Shape5(2,20,30,4,1));
    testCompressedArray<5, vigra::Int64 >(vigra::Shape5(2,20,30,4,1));
}
