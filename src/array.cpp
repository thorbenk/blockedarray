#include <bw/array.h>
#include <bw/extern_templates.h>

namespace BW {
    template class Array<2, vigra::UInt8>; 
    template class Array<3, vigra::UInt8>; 
    template class Array<4, vigra::UInt8>; 
    template class Array<5, vigra::UInt8>; 

    template class Array<2, vigra::UInt32>; 
    template class Array<3, vigra::UInt32>; 
    template class Array<4, vigra::UInt32>; 
    template class Array<5, vigra::UInt32>; 
    
    template class Array<2, float>; 
    template class Array<3, float>; 
    template class Array<4, float>; 
    template class Array<5, float>; 
}
