#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h" 

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>

#include <bw/connectedcomponents.h>
#include <bw/thresholding.h>
#include <bw/channelselector.h>

#include "blockwisecc_py.h"

#include <bw/extern_templates.h>

using namespace BW;

template<int N>
struct PyConnectedComponents {
    typedef ConnectedComponents<N> BCC;
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
    typedef ConnectedComponents<N> BCC;
    typedef Thresholding<N, T> BWT;
    typedef ChannelSelector<N+1, T> BWCS;
    typedef PyConnectedComponents<N> PyBCC;
    typedef SourceHDF5<N, T> HDF5BP_T;
   
    std::stringstream n; n << N;
    
    std::stringstream fullModname; fullModname << "_blockedarray.dim" << N;
    std::stringstream modName; modName << "dim" << N;
    
    //see: http://isolation-nation.blogspot.de/2008/09/packages-in-python-extension-modules.html
    object module(handle<>(borrowed(PyImport_AddModule(fullModname.str().c_str()))));
    scope().attr(modName.str().c_str()) = module;
    scope s = module;
    
    ExportV<N, typename BCC::V>::export_();
    
    class_<Source<N, T> >("Source", no_init);
    class_<Sink<N, T> >("Sink", no_init);
    
    class_<SourceHDF5<N, T>, bases<Source<N, T> > >("SourceHDF5",
        init<std::string, std::string>())
    ;
    class_<SinkHDF5<N, T>, bases<Sink<N, T> > >("SinkHDF5",
        init<std::string, std::string>())
    ;
    
    class_<BWT>("Thresholding",
        init<Source<N,T>*, typename BWT::V>())
        .def("run", vigra::registerConverters(&BWT::run),
             (arg("threshold"), arg("ifLower"), arg("ifHigher"), arg("sink")))
    ;
        
    class_<BWCS>("ChannelSelector",
        init<Source<N+1,T>*, typename BWCS::V>())
        .def("run", vigra::registerConverters(&BWCS::run),
            (arg("dimension"), arg("channel"), arg("sink")))
    ;
   
    class_<BCC>("ConnectedComponents",
        init<Source<N, vigra::UInt8>*, typename BCC::V>())
        .def("writeResult", &BCC::writeResult,
             (arg("hdf5file"), arg("hdf5group"), arg("compression")=1))
    ;
}

void export_blockwiseCC() {
    blockwiseCC<2, float>();
    blockwiseCC<3, float>();
}  
