#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h" 

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>

//#define DEBUG_PRINTS

#include "blockedarray.h"

template<int N, class T>
struct PyBlockedArray {
    typedef BlockedArray<N, T> BA;
    
    static void readSubarray(BA& ba,
                             typename BA::difference_type p, typename BA::difference_type q,
                             vigra::NumpyArray<N, T> out
    ) {
        ba.readSubarray(p, q, out);
    }
    
    static void writeSubarray(BA& ba,
                              typename BA::difference_type p, typename BA::difference_type q,
                              vigra::NumpyArray<N, T> a
    ) {
        ba.writeSubarray(p, q, a);
    }
    
    static void sliceToPQ(boost::python::tuple sl,
                          typename BA::difference_type &p, typename BA::difference_type &q)
    {
        vigra_precondition(boost::python::len(sl)==N, "tuple has wrong length");
        for(int k=0; k<N; ++k) {
            boost::python::slice s = boost::python::extract<boost::python::slice>(sl[k]);
            p[k] = boost::python::extract<int>(s.start()); 
            q[k] = boost::python::extract<int>(s.stop()); 
        }
    }
    
    static vigra::NumpyAnyArray getitem(BA& ba, boost::python::tuple sl) {
        typename BA::difference_type p,q;
        sliceToPQ(sl, p, q);
        vigra::NumpyArray<N,T> out(q-p);
        ba.readSubarray(p, q, out);
        return out;
    }
    
    static void setitem(BA& ba, boost::python::tuple sl, vigra::NumpyArray<N,T> a) {
        typename BA::difference_type p,q;
        sliceToPQ(sl, p, q);
        ba.writeSubarray(p, q, a);
    }
    
    static boost::python::tuple dirtyBlocks(BA& ba, typename BA::difference_type p, typename BA::difference_type q) {
        typename BA::BlockList bL = ba.dirtyBlocks(p, q);
        vigra::NumpyArray<2, vigra::UInt32> start(vigra::Shape2(bL.size(), N));
        vigra::NumpyArray<2, vigra::UInt32> stop(vigra::Shape2(bL.size(), N));
        for(int i=0; i<bL.size(); ++i) {
            typename BA::difference_type pp, qq;
            ba.blockBounds(bL[i], pp, qq);
            for(int j=0; j<N; ++j) {
                start(i,j) = pp[j];
                stop(i,j) = qq[j];
            }
        }
        return boost::python::make_tuple(start, stop);
    }
};

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

template<int N, class T>
void export_blockedArray() {
    typedef BlockedArray<N, T> BA;
    
    using namespace boost::python;
    using namespace vigra;
    
    std::stringstream name; name << "BlockedArray" << N << DtypeName<T>::dtypeName();
    
    class_<BA>(name.str().c_str(), init<typename BA::difference_type>())
        .def("averageCompressionRatio", registerConverters(&BA::averageCompressionRatio))
        .def("numBlocks", registerConverters(&BA::numBlocks))
        .def("sizeBytes", registerConverters(&BA::sizeBytes))
        .def("writeSubarray", registerConverters(&PyBlockedArray<N,T>::writeSubarray))
        .def("readSubarray", registerConverters(&PyBlockedArray<N,T>::readSubarray))
        .def("__getitem__", registerConverters(&PyBlockedArray<N,T>::getitem))
        .def("__setitem__", registerConverters(&PyBlockedArray<N,T>::setitem))
        .def("deleteSubarray", registerConverters(&BA::deleteSubarray))
        .def("setDirty", registerConverters(&BA::setDirty))
        .def("dirtyBlocks", registerConverters(&PyBlockedArray<N,T>::dirtyBlocks))
    ;
}

void export_blockedArray() {
    export_blockedArray<2, vigra::UInt8>();
    export_blockedArray<3, vigra::UInt8>();
    export_blockedArray<4, vigra::UInt8>();
    export_blockedArray<5, vigra::UInt8>();
    
    export_blockedArray<2, vigra::UInt32>();
    export_blockedArray<3, vigra::UInt32>();
    export_blockedArray<4, vigra::UInt32>();
    export_blockedArray<5, vigra::UInt32>();
    
    export_blockedArray<2, vigra::Int64>();
    export_blockedArray<3, vigra::Int64>();
    export_blockedArray<4, vigra::Int64>();
    export_blockedArray<5, vigra::Int64>();
    
    export_blockedArray<2, float>();
    export_blockedArray<3, float>();
    export_blockedArray<4, float>();
    export_blockedArray<5, float>();
    
}  
