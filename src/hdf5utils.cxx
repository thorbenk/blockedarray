#include <bw/hdf5utils.h>

namespace BW {

// uint8
template<>
hid_t H5Type<uint8_t>::get_STD_LE() {
    return H5T_STD_U8LE;
}
template<>
hid_t H5Type<uint8_t>::get_NATIVE() {
    return H5T_NATIVE_UINT8;
}

// uint32
template<>
hid_t H5Type<uint32_t>::get_STD_LE() {
    return H5T_STD_U32LE;
}
template<>
hid_t H5Type<uint32_t>::get_NATIVE() {
    return H5T_NATIVE_UINT32;
}

// float32
template<>
hid_t H5Type<float>::get_STD_LE() {
    return H5T_NATIVE_FLOAT;
}
template<>
hid_t H5Type<float>::get_NATIVE() {
    return H5T_NATIVE_FLOAT;
}

template<>
void H5A<bool>::write(hid_t f, const char* name, const bool& a) {
    hsize_t one = 1;
    hid_t space = H5Screate_simple(1, &one, NULL); 
    hid_t attr = H5Acreate(f, name, H5T_STD_U8LE, space, H5P_DEFAULT, H5P_DEFAULT);
    unsigned char d = a ? 1 : 0;
    H5Awrite(attr, H5T_NATIVE_UINT8, &d);
    H5Aclose(attr);
    H5Sclose(space);
}

template<>
bool H5A<bool>::read(hid_t f, const char* name) {
    hid_t attr = H5Aopen(f, name, H5P_DEFAULT);
    uint8_t d;
    H5Aread(attr, H5T_NATIVE_UINT8, &d);
    H5Aclose(attr);
    return d > 0;
}

}
