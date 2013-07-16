#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h" 

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>

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
};


template<int N, class T>
void export_blockedArray() {
    typedef BlockedArray<N, T> BA;
    
    using namespace boost::python;
    using namespace vigra;
    
    class_<BA>("BlockedArray", init<typename BA::difference_type>())
        .def("averageCompressionRatio", registerConverters(&BA::averageCompressionRatio))
        .def("numBlocks", registerConverters(&BA::numBlocks))
        .def("sizeBytes", registerConverters(&BA::sizeBytes))
        .def("writeSubarray", registerConverters(&PyBlockedArray<N,T>::writeSubarray))
        .def("readSubarray", registerConverters(&PyBlockedArray<N,T>::readSubarray))
        .def("__getitem__", registerConverters(&PyBlockedArray<N,T>::getitem))
        .def("__setitem__", registerConverters(&PyBlockedArray<N,T>::setitem))
    ;
}

void export_blockedArray() {
    export_blockedArray<3, vigra::UInt8>();
}  