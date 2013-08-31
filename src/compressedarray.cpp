#include <bw/array.h>
#include <bw/extern_templates.h>

namespace BW {
    template class CompressedArray<2, vigra::UInt8>; 
    template class CompressedArray<3, vigra::UInt8>; 
    template class CompressedArray<4, vigra::UInt8>; 
    template class CompressedArray<5, vigra::UInt8>; 

    template class CompressedArray<2, vigra::UInt32>; 
    template class CompressedArray<3, vigra::UInt32>; 
    template class CompressedArray<4, vigra::UInt32>; 
    template class CompressedArray<5, vigra::UInt32>; 
    
    template class CompressedArray<2, float>; 
    template class CompressedArray<3, float>; 
    template class CompressedArray<4, float>; 
    template class CompressedArray<5, float>; 
}
