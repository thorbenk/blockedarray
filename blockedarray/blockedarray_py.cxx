#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h" 

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>
#include <vigra/python_utility.hxx>

//#define DEBUG_PRINTS

#include <bw/array.h>

#include "dtypename.h"

#include <bw/extern_templates.h>

using namespace BW;

template<int N, typename T>
boost::python::tuple blockListToPython(
    const Array<N,T>& ba,
    const typename Array<N,T>::BlockList& bL) {
    vigra::NumpyArray<2, vigra::UInt32> start(vigra::Shape2(bL.size(), N));
    vigra::NumpyArray<2, vigra::UInt32> stop(vigra::Shape2(bL.size(), N));
    for(int i=0; i<bL.size(); ++i) {
        typename Array<N,T>::V pp, qq;
        ba.blockBounds(bL[i], pp, qq);
        for(int j=0; j<N; ++j) {
            start(i,j) = pp[j];
            stop(i,j) = qq[j];
        }
    }
    return boost::python::make_tuple(start, stop);
}

template<int N, typename T>
boost::python::list voxelValuesToPython(
    const typename Array<N,T>::VoxelValues& vv
) {
    typedef vigra::NumpyArray<1, uint32_t> PyCoord;
    typedef vigra::NumpyArray<1, T> PyVal;
    typedef vigra::Shape1 Sh;
    
    PyCoord coords[N];
    for(int i=0; i<N; ++i) {
        coords[i].reshape(Sh(vv.first.size()));
    }
    PyVal vals(Sh(vv.first.size()));
    for(size_t j=0; j<N; ++j) {
        PyCoord& c = coords[j];
        for(size_t i=0; i<vv.first.size(); ++i) {
            c[i] = vv.first[i][j];
        }
    }
    for(size_t i=0; i<vv.first.size(); ++i) {
        vals[i] = vv.second[i];
    }
    
    //convert to python list
    boost::python::list l;
    for(int i=0; i<N; ++i) {
        l.append(coords[i]);
    }
    l.append(vals);
    return l;
}

template<int N, class T>
struct PyBlockedArray {
    typedef Array<N, T> BA;
    typedef typename BA::V V;
    
    static void readSubarray(BA& ba, V p, V q, vigra::NumpyArray<N, T> out
    ) {
        ba.readSubarray(p, q, out);
    }
    
    static void writeSubarray(BA& ba, V p, V q, vigra::NumpyArray<N, T> a
    ) {
        ba.writeSubarray(p, q, a);
    }
    
    static void writeSubarrayNonzero(BA& ba, V p, V q, vigra::NumpyArray<N, T> a, T writeAsZero
    ) {
        ba.writeSubarrayNonzero(p, q, a, writeAsZero);
    }
    
    static void sliceToPQ(boost::python::tuple sl, V &p, V &q) {
        vigra_precondition(boost::python::len(sl)==N, "tuple has wrong length");
        for(int k=0; k<N; ++k) {
            boost::python::slice s = boost::python::extract<boost::python::slice>(sl[k]);
            p[k] = boost::python::extract<int>(s.start()); 
            q[k] = boost::python::extract<int>(s.stop()); 
        }
    }
    
    static vigra::NumpyAnyArray getitem(BA& ba, boost::python::tuple sl) {
        V p,q;
        sliceToPQ(sl, p, q);
        vigra::NumpyArray<N,T> out(q-p);
        ba.readSubarray(p, q, out);
        return out;
    }
    
    static void setDirty(BA& ba, boost::python::tuple sl, bool dirty) {
        V p,q;
        sliceToPQ(sl, p, q);
        ba.setDirty(p,q,dirty);
    }
    
    static void setitem(BA& ba, boost::python::tuple sl, vigra::NumpyArray<N,T> a) {
        V p,q;
        sliceToPQ(sl, p, q);
        ba.writeSubarray(p, q, a);
    }

    static PyObject* blockShape(BA& ba) {
    	return shapeToPythonTuple( ba.blockShape() ).release();
    }

    static boost::python::tuple blocks(BA& ba, V p, V q) {
        return blockListToPython(ba, ba.blocks(p, q));
    }
    
    static boost::python::tuple dirtyBlocks(BA& ba, V p, V q) {
        return blockListToPython(ba, ba.dirtyBlocks(p, q));
    }
    
    static boost::python::tuple minMax(const BA& ba) {
        std::pair<T,T> mm = ba.minMax();
        return boost::python::make_tuple(mm.first, mm.second); 
    }
    
    static boost::python::list nonzero(const BA& ba) {
        return voxelValuesToPython<N,T>(ba.nonzero());
    }
    
    static void applyRelabeling(BA& ba, vigra::NumpyArray<1, T>& relabeling) {
        ba.applyRelabeling(relabeling);
    }
};

template<int N, class T>
void export_blockedArray() {
    typedef Array<N, T> BA;
    typedef PyBlockedArray<N,T> PyBA;
    
    using namespace boost::python;
    using namespace vigra;
    
    std::stringstream name; name << "BlockedArray" << N << DtypeName<T>::dtypeName();
    
    class_<BA>(name.str().c_str(), init<typename BA::V>())
        .def("setDeleteEmptyBlocks", &BA::setDeleteEmptyBlocks,
             (arg("deleteEmpty")))
        .def("setCompressionEnabled", &BA::setCompressionEnabled,
             (arg("enableCompression")))
        .def("setMinMaxTrackingEnabled", &BA::setMinMaxTrackingEnabled,
             (arg("enableMinMaxTracking")))
        .def("setManageCoordinateLists", &BA::setManageCoordinateLists,
             (arg("manageCoordinateLists")))
        .def("minMax", &PyBA::minMax)
        .def("averageCompressionRatio", &BA::averageCompressionRatio)
        .def("numBlocks", &BA::numBlocks)
        .def("sizeBytes", &BA::sizeBytes)
        .def("blockShape", &PyBA::blockShape)
        .def("writeSubarray", registerConverters(&PyBA::writeSubarray))
        .def("writeSubarrayNonzero", registerConverters(&PyBA::writeSubarrayNonzero),
            (arg("p"), arg("q"), arg("a"), arg("writeAsZero")))
        .def("readSubarray", registerConverters(&PyBA::readSubarray))
        .def("deleteSubarray", registerConverters(&BA::deleteSubarray))
        .def("applyRelabeling", registerConverters(&PyBA::applyRelabeling),
            (arg("relabeling")))
        .def("__getitem__", registerConverters(&PyBA::getitem))
        .def("__setitem__", registerConverters(&PyBA::setitem))
        .def("setDirty", registerConverters(&PyBA::setDirty))
        .def("blocks", registerConverters(&PyBA::blocks))
        .def("dirtyBlocks", registerConverters(&PyBA::dirtyBlocks))
        .def("nonzero", registerConverters(&PyBA::nonzero))
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
    
    export_blockedArray<2, float>();
    export_blockedArray<3, float>();
    export_blockedArray<4, float>();
    export_blockedArray<5, float>();
    
}  
