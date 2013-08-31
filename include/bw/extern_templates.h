#ifndef BW_EXTERN_TEMPLATES
#define BW_EXTERN_TEMPLATES

#ifdef HAS_EXTERN_TEMPLATES

#include <bw/roi.h>
#include <vigra/multi_array.hxx>
#include <bw/array.h>

namespace BW {

//#ifdef BW_ROI_H
extern template class Roi<2>;
extern template class Roi<3>;
extern template class Roi<4>;
extern template class Roi<5>;
//#endif

//#ifdef BW_ARRAY_H
extern template class Array<2, vigra::UInt8>; 
extern template class Array<3, vigra::UInt8>; 
extern template class Array<4, vigra::UInt8>; 
extern template class Array<5, vigra::UInt8>; 

extern template class Array<2, vigra::UInt32>; 
extern template class Array<3, vigra::UInt32>; 
extern template class Array<4, vigra::UInt32>; 
extern template class Array<5, vigra::UInt32>; 

extern template class Array<2, float>; 
extern template class Array<3, float>; 
extern template class Array<4, float>; 
extern template class Array<5, float>; 
//#endif /*BW_ARRAY_H*/


} /* namespace BW */

//#ifdef VIGRA_MULTI_ARRAY_HXX
extern template class vigra::MultiArray<2, vigra::UInt8>;
extern template class vigra::MultiArray<3, vigra::UInt8>;
extern template class vigra::MultiArray<4, vigra::UInt8>;
extern template class vigra::MultiArray<5, vigra::UInt8>;
extern template class vigra::MultiArrayView<2, vigra::UInt8>;
extern template class vigra::MultiArrayView<3, vigra::UInt8>;
extern template class vigra::MultiArrayView<4, vigra::UInt8>;
extern template class vigra::MultiArrayView<5, vigra::UInt8>;

extern template class vigra::MultiArray<2, vigra::UInt32>;
extern template class vigra::MultiArray<3, vigra::UInt32>;
extern template class vigra::MultiArray<4, vigra::UInt32>;
extern template class vigra::MultiArray<5, vigra::UInt32>;
extern template class vigra::MultiArrayView<2, vigra::UInt32>;
extern template class vigra::MultiArrayView<3, vigra::UInt32>;
extern template class vigra::MultiArrayView<4, vigra::UInt32>;
extern template class vigra::MultiArrayView<5, vigra::UInt32>;

extern template class vigra::MultiArray<2, float>;
extern template class vigra::MultiArray<3, float>;
extern template class vigra::MultiArray<4, float>;
extern template class vigra::MultiArray<5, float>;
extern template class vigra::MultiArrayView<2, float>;
extern template class vigra::MultiArrayView<3, float>;
extern template class vigra::MultiArrayView<4, float>;
extern template class vigra::MultiArrayView<5, float>;
//#endif

#endif /*HAS_EXTERN_TEMPLATES*/

#endif /* BW_EXTERN_TEMPLATES */
