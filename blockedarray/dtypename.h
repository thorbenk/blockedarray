#ifndef BW_DTYPENAME_H
#define BW_DTYPENAME_H

#include <vigra/sized_int.hxx>

template<class T>
struct DtypeName {
    static void dtypeName() {
        throw std::runtime_error("not specialized");
    }
};

template<>
struct DtypeName<vigra::UInt8> {
    static std::string dtypeName() {
        return "uint8";
    }
};
template<>
struct DtypeName<float> {
    static std::string dtypeName() {
        return "float32";
    }
};
template<>
struct DtypeName<double> {
    static std::string dtypeName() {
        return "float64";
    }
};
template<>
struct DtypeName<vigra::UInt32> {
    static std::string dtypeName() {
        return "uint32";
    }
};
template<>
struct DtypeName<vigra::UInt64> {
    static std::string dtypeName() {
        return "uint64";
    }
};
template<>
struct DtypeName<vigra::Int64> {
    static std::string dtypeName() {
        return "int64";
    }
};
template<>
struct DtypeName<vigra::Int32> {
    static std::string dtypeName() {
        return "int32";
    }
};

#endif /*BW_DTYPENAME_H*/
