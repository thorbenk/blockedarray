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

} /* namespace Hdf5 */

#endif /* BW_HDF5UTILS_H */