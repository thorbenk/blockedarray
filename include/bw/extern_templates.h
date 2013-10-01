/************************************************************************/
/*                                                                      */
/*    Copyright 2013 by Thorben Kroeger                                 */
/*    thorben.kroeger@iwr.uni-heidelberg.de                             */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

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

extern template class CompressedArray<2, vigra::UInt8>;
extern template class CompressedArray<3, vigra::UInt8>;
extern template class CompressedArray<4, vigra::UInt8>;
extern template class CompressedArray<5, vigra::UInt8>;

extern template class CompressedArray<2, vigra::UInt32>;
extern template class CompressedArray<3, vigra::UInt32>;
extern template class CompressedArray<4, vigra::UInt32>;
extern template class CompressedArray<5, vigra::UInt32>;

extern template class CompressedArray<2, float>;
extern template class CompressedArray<3, float>;
extern template class CompressedArray<4, float>;
extern template class CompressedArray<5, float>;

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
