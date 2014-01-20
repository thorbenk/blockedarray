#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h"

#include <boost/python.hpp>
#include <boost/python/slice.hpp>
#include <boost/tuple/tuple.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>

#include <bw/connectedcomponents.h>
#include <bw/thresholding.h>
#include <bw/channelselector.h>

#include "adapters_py.h"

#include <bw/extern_templates.h>

using namespace BW;


/* ROI conversion */

template<int N>
boost::python::list tinyVecToList(const typename Roi<N>::V &vec)
{
    boost::python::object iterator = boost::python::iterator<typename Roi<N>::V>()(vec);
    return boost::python::list(iterator);
}

template<int N>
struct Roi_to_python_tuple
{
    typedef typename Roi<N>::V TinyVec;
    static PyObject* convert(const Roi<N> &roi)
    {
        // p
        boost::python::list p = tinyVecToList<N>(roi.p);
        // q
        boost::python::list q = tinyVecToList<N>(roi.q);
        
        boost::python::tuple t = boost::python::make_tuple(p, q);
        return boost::python::incref(t.ptr());
    }
};

void registerConverters()
{
    boost::python::to_python_converter<Roi<1>, Roi_to_python_tuple<1> >();
    boost::python::to_python_converter<Roi<2>, Roi_to_python_tuple<2> >();
    boost::python::to_python_converter<Roi<3>, Roi_to_python_tuple<3> >();
    boost::python::to_python_converter<Roi<4>, Roi_to_python_tuple<4> >();
    boost::python::to_python_converter<Roi<5>, Roi_to_python_tuple<5> >();
    
    // I really don't know why this is not called in vigra
    vigra::NumpyArrayConverter<vigra::NumpyArray<3, ConnectedComponents<3>::LabelType> >();
    vigra::NumpyArrayConverter<vigra::NumpyArray<1, npy_int32> >();
}


template<int N, class T>
struct PySourceABC : Source<N,T>, boost::python::wrapper<Source<N,T> > 
{
public:
    
    typedef typename Source<N,T>::V TinyVec;
    
    PySourceABC() {};
    virtual ~PySourceABC() {};

    void setRoi(Roi<N> roi)
    {
        this->pySetRoi(roi);
    }
    
    TinyVec shape() const 
    {
        return this->pyShape();
    }
    
    bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const 
    {
        bool ret;
        
        //temporary NumpyArray, because MultiArrayView is not convertible to python
        vigra::NumpyArray<N,T> tempArray(roi.q-roi.p);
        
        if (!this->pyReadBlock(roi, tempArray))
            return false;
        
        block.copy(tempArray);
        
        return true;
    }
    
    void pySetRoi(Roi<N> roi)
    {
        this->get_override("pySetRoi")(roi);
    }

    TinyVec pyShape() const 
    {
        return this->get_override("pyShape")();
    };

    bool pyReadBlock(Roi<N> roi, vigra::NumpyArray<N,T>& block) const 
    {
        return this->get_override("pyReadBlock")(roi, block);
    };
};

template<int N, class T>
struct PySinkABC : Sink<N,T>, boost::python::wrapper<Sink<N,T> > {
    public:
    typedef typename Roi<N>::V V;

    PySinkABC() {};
    virtual ~PySinkABC() {};

    bool writeBlock(Roi<N> roi, const vigra::MultiArrayView<N,T>& block)
    {
        vigra::NumpyArray<N,T> tempArray(block);
        return this->pyWriteBlock(roi, tempArray);
    }
    
    bool pyWriteBlock(Roi<N> roi, const vigra::NumpyArray<N,T>& block)
    {
        return this->get_override("pyWriteBlock")(roi, block);
    };
    
    /* Accessor functions for the shapes */
    boost::python::list getShape() const
    {
        return tinyVecToList<N>(this->shape_);
    }
    
    void setShape(V shape)
    {
        this->shape_ = shape;
    }

    boost::python::list getBlockShape() const
    {
        return tinyVecToList<N>(this->blockShape_);
    }
    
    void setBlockShape(V shape)
    {
        this->blockShape_ = shape;
    }
};



template<int N, class T>
void exposeSource(const char* exposedName) {
    using namespace boost::python;
    
    class_<PySourceABC<N,T>, boost::noncopyable>(exposedName)
        .def("pySetRoi", pure_virtual(&PySourceABC<N,T>::pySetRoi))
        .def("pyShape", pure_virtual(&PySourceABC<N,T>::pyShape))
        .def("pyReadBlock", pure_virtual(&PySourceABC<N,T>::pyReadBlock))
        ;
}

template<int N, class T>
void exposeSink(const char* exposedName) {
    using namespace boost::python;
    
    class_<PySinkABC<N,T>, boost::noncopyable>(exposedName)
        .def("pyWriteBlock", pure_virtual(&PySinkABC<N,T>::pyWriteBlock))
        .add_property("shape", &PySinkABC<N,T>::getShape, &PySinkABC<N,T>::setShape)
        .add_property("blockShape", &PySinkABC<N,T>::getBlockShape, &PySinkABC<N,T>::setBlockShape)
        ;
}

void export_adapters() {
    exposeSource<3,vigra::UInt8>("Source3U8");
    exposeSink<3,ConnectedComponents<3>::LabelType>("Sink3");
    registerConverters();
}
