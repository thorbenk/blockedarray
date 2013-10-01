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

#ifndef BW_HDF5UTILS_H
#define BW_HDF5UTILS_H

//these defines are copied from vigra/hdf5impex.hxx
#define H5Gcreate_vers 2
#define H5Gopen_vers 2
#define H5Dopen_vers 2
#define H5Dcreate_vers 2
#define H5Acreate_vers 2

#include "hdf5.h"

#if (H5_VERS_MAJOR == 1 && H5_VERS_MINOR <= 6)
# ifndef H5Gopen
#   define H5Gopen(a, b, c) H5Gopen(a, b)
# endif
# ifndef H5Gcreate
#  define H5Gcreate(a, b, c, d, e) H5Gcreate(a, b, 1)
# endif
# ifndef H5Dopen
#  define H5Dopen(a, b, c) H5Dopen(a, b)
# endif
# ifndef H5Dcreate
#  define H5Dcreate(a, b, c, d, e, f, g) H5Dcreate(a, b, c, d, f)
# endif
# ifndef H5Acreate
#  define H5Acreate(a, b, c, d, e, f) H5Acreate(a, b, c, d, e)
# endif
# ifndef H5Pset_obj_track_times
#  define H5Pset_obj_track_times(a, b) do {} while (0)
# endif
# include <H5LT.h>
#else
# include <hdf5_hl.h>
#endif

namespace BW {

template<typename T>
struct H5Type {
    static hid_t get_STD_LE();
    static hid_t get_NATIVE();
};

template<typename T>
struct H5A {
    static void write(hid_t f, const char* name, const T& a);
    static T read(hid_t f, const char* name);
};

template<typename T>
struct H5D {
    static void readShape(hid_t f, const char* name, hsize_t* shape);

    static void read(hid_t f, const char* name, int N, hsize_t*& shape, T*& data);
    static void write(hid_t f, const char* name, int N, hsize_t* shape, T* data);
};

template<typename T>
void H5D<T>::readShape(hid_t f, const char* name, hsize_t* shape) {
    hid_t dataset  = H5Dopen(f, name, H5P_DEFAULT);
    hid_t filetype = H5Dget_type(dataset);
    hid_t space    = H5Dget_space(dataset);
    H5Sget_simple_extent_dims(space, shape, NULL);
    H5Sclose(space);
    H5Tclose(filetype);
    H5Dclose(dataset);
}

template<typename T>
void H5D<T>::read(hid_t f, const char* name, int N, hsize_t*& shape, T*& data) {
    throw std::runtime_error("xxxxx ttttt");

    hid_t dataset  = H5Dopen(f, name, H5P_DEFAULT);
    hid_t filetype = H5Dget_type(dataset);
    hid_t space    = H5Dget_space(dataset);

    H5Sget_simple_extent_dims(space, shape, NULL);
    size_t sz = 1;
    for(size_t i=0; i<N; ++i) { sz *= shape[i]; }
    data = new T[sz];
    H5Dread(dataset, H5Type<T>::get_NATIVE(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    H5Sclose(space);
    H5Tclose(filetype);
    H5Dclose(dataset);
}

template<typename T>
void H5D<T>::write(hid_t f, const char* name, int N, hsize_t* shape, T* data) {
    throw std::runtime_error("xxxxx ttttt");

    hid_t space   = H5Screate_simple(N, shape, NULL);
    hid_t dataset = H5Dcreate(f, name, H5Type<T>::get_STD_LE(), space,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dataset, H5Type<T>::get_NATIVE(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Dclose(dataset);
    H5Sclose(space);
}



} /* namespace Hdf5 */

#endif /* BW_HDF5UTILS_H */