#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h" 

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>

#include "blockwisecc_py.h"
#include "blockwisecc.h"

template<int N, class T>
struct PyBlockwiseConnectedComponents {
    typedef BlockwiseConnectedComponents<N, T> BCC;
    static void getResult(BCC& bcc, vigra::NumpyArray<N, T> out) {
        bcc.getResult(out);
    }
};

template<int N, class V>
struct ExportV {
    static void export_();
};

template<class V>
struct ExportV<2, V> {
    static void export_() {
        using namespace boost::python;
        class_<V>("V", init<int, int>());
    }
};


template<class V>
struct ExportV<3, V> {
    static void export_() {
        using namespace boost::python;
        class_<V>("V", init<int, int, int>());
    }
};

template<class V>
struct ExportV<4, V> {
    static void export_() {
        using namespace boost::python;
        class_<V>("V", init<int, int, int, int>());
    }
};

template<int N, class T>
void blockwiseCC() {
    
    using namespace boost::python;
    typedef BlockwiseConnectedComponents<N, T> BCC;
    typedef BlockwiseThresholding<N, T> BWT;
    typedef BlockwiseChannelSelector<N+1, T> BWCS;
    typedef PyBlockwiseConnectedComponents<N,T> PyBCC;
    typedef HDF5BlockProvider<N, T> HDF5BP_T;
   
    std::stringstream n; n << N;
    
    std::stringstream fullModname; fullModname << "_blockedarray.dim" << N;
    std::stringstream modName; modName << "dim" << N;
    
    //see: http://isolation-nation.blogspot.de/2008/09/packages-in-python-extension-modules.html
    object module(handle<>(borrowed(PyImport_AddModule(fullModname.str().c_str()))));
    scope().attr(modName.str().c_str()) = module;
    scope s = module;
    
    ExportV<N, typename BCC::V>::export_();
    
    class_<BWT>("BlockwiseThresholding",
        init<std::string, std::string, typename BWT::V>())
        .def("run", vigra::registerConverters(&BWT::run))
    ;
    
    class_<BWCS>("BlockwiseChannelSelector",
        init<std::string, std::string, typename BWCS::V>())
        .def("run", vigra::registerConverters(&BWCS::run))
    ;
   
    class_<BlockProvider<N, T> >("BlockProvider", no_init);
    
    class_<HDF5BlockProvider<N, T>, bases<BlockProvider<N, T> > >("HDF5BlockProvider",
        init<std::string, std::string>())
    ;
    
    class_<BCC>("BlockwiseConnectedComponents",
        init<BlockProvider<N, T>*, typename BCC::V>())
        .def("writeResult", &BCC::writeResult)
    ;
}

void export_blockwiseCC() {
    blockwiseCC<2, float>();
    blockwiseCC<3, float>();
}  
