#include <vigra/multi_array.hxx>

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
void H5A<size_t>::write(hid_t f, const char* name, const size_t& a) {
    static const hsize_t one = 1;
    hid_t space = H5Screate_simple(1, &one, NULL); 
    hid_t attr  = H5Acreate(f, name, H5T_STD_U64LE, space, H5P_DEFAULT, H5P_DEFAULT);
    uint64_t s = static_cast<uint64_t>(a); 
    H5Awrite(attr, H5T_NATIVE_UINT64, &s);
    H5Aclose(attr);
    H5Sclose(space);
}

template<>
void H5A<bool>::write(hid_t f, const char* name, const bool& a) {
    static const hsize_t one = 1;
    hid_t space = H5Screate_simple(1, &one, NULL); 
    hid_t attr = H5Acreate(f, name, H5T_STD_U8LE, space, H5P_DEFAULT, H5P_DEFAULT);
    unsigned char d = a ? 1 : 0;
    H5Awrite(attr, H5T_NATIVE_UINT8, &d);
    H5Aclose(attr);
    H5Sclose(space);
}

template<>
size_t H5A<size_t>::read(hid_t f, const char* name) {
    hid_t attr = H5Aopen(f, name, H5P_DEFAULT);
    uint64_t s;
    H5Aread(attr, H5T_NATIVE_UINT64, &s);
    H5Aclose(attr);
    return static_cast<size_t>(s);
}

template<>
bool H5A<bool>::read(hid_t f, const char* name) {
    hid_t attr = H5Aopen(f, name, H5P_DEFAULT);
    uint8_t d;
    H5Aread(attr, H5T_NATIVE_UINT8, &d);
    H5Aclose(attr);
    return d > 0;
}

} /* namespace BW */
