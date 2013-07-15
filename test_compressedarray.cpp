#include <iostream>

#include "compressedarray.h"

int main() {
    vigra::MultiArray<1, int> test(vigra::Shape1(10));
    test[0] = 1;
    test[1] = 2;
    test[9] = 9;
    
    size_t input_length = sizeof(int)*test.size();

    char* output = new char[snappy::MaxCompressedLength(input_length)];
    size_t output_length;
    snappy::RawCompress((char*)(test.data()), input_length, output, &output_length);
    std::cout << output_length << std::endl;
    delete [] output;

    CompressedArray<1, int> c(test);
    std::cout << "after constructor" << std::endl;
    std::cout << "  size           " << c.size() << std::endl;
    std::cout << "  is compressed? " << c.isCompressed() << std::endl; 
    std::cout << "  size in bytes: " << c.currentSizeBytes() << std::endl;

    std::cout << "uncompress ..." << std::endl;
    c.uncompress();
    std::cout << "  size in bytes: " << c.currentSizeBytes() << std::endl;
    std::cout << "  is compressed? " << c.isCompressed() << std::endl; 

    std::cout << "compress ..." << std::endl;
    c.compress();
    std::cout << "  size in bytes: " << c.currentSizeBytes() << std::endl;
    std::cout << "  is compressed? " << c.isCompressed() << std::endl; 

    auto d = c.readArray();

    std::cout << d[0] << std::endl;
    std::cout << d[1] << std::endl;

    vigra::MultiArrayShape<3>::type sh(20,20,20);
    vigra::MultiArray<3, int> a(vigra::Shape3(100,100,100));
}