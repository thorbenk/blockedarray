#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h" 

#include <boost/python.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>

#include "blockedarray.h"

typedef BlockedArray<3, vigra::UInt8, CompressedArray<3, vigra::UInt8> > BA;

struct PyBlockedArray {
    static void readSubarray(BA& ba,
                             BA::difference_type p, BA::difference_type q,
                             vigra::NumpyArray<3, unsigned char> out
    ) {
        ba.readSubarray(p, q, out);
    }
    
    static void writeSubarray(BA& ba,
                              BA::difference_type p, BA::difference_type q,
                              vigra::NumpyArray<3, unsigned char> a
    ) {
        ba.writeSubarray(p, q, a);
    }
};

void test(vigra::NumpyArray<3, unsigned char> t) {
}

void export_blockedArray() {
    using namespace boost::python;
    using namespace vigra;
    
    NumpyArrayConverter< vigra::NumpyArray<3, unsigned char> >();
    
    def("test", registerConverters(&test));
    
    class_<BA>("BlockedArray", init<typename BA::difference_type>())
        .def("averageCompressionRatio", registerConverters(&BA::averageCompressionRatio))
        .def("numBlocks", registerConverters(&BA::numBlocks))
        .def("sizeBytes", registerConverters(&BA::sizeBytes))
        .def("writeSubarray", registerConverters(&PyBlockedArray::writeSubarray))
        .def("readSubarray", registerConverters(&PyBlockedArray::readSubarray))
    ;
}  