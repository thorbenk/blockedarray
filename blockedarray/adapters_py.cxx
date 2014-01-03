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

#include "adapters_py.h"

#include <bw/extern_templates.h>

using namespace BW;

/*

template<int N, class T>
class PySourceABC : public Source<N,T>
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
        
        //temporary NumpyArray
        vigra::NumpyArray<N,T> tempArray(roi.q-roi.p);
        
        if (!this->pyReadBlock(roi, tempArray))
            return false;
        
        //TODO copy data to MAV
        
        return true;
    }
    
    virtual void pySetRoi(Roi<N> roi) = 0;
    virtual typename Source<N,T>::V pyShape() const  = 0;
    virtual bool pyReadBlock(Roi<N> roi, vigra::NumpyArray<N,T>& block) const = 0;
};
*/

/* expose Source / Sink to python with inheritance callable from C++ */

/*
template<int N, class T>
struct PySourceABCWrap : PySourceABC<N,T>,
                         boost::python::wrapper<PySourceABC<N,T> >,
                         boost::python::wrapper<Source<N,T> > 
{
public:
    
    typedef typename Source<N,T>::V TinyVec;
    
    PySourceABCWrap() {};
    virtual ~PySourceABCWrap() {};

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
        return this->get_override("pyReadBlock");
    };
};
*/

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
        
        //temporary NumpyArray
        vigra::NumpyArray<N,T> tempArray(roi.q-roi.p);
        
        if (!this->pyReadBlock(roi, tempArray))
            return false;
        
        //TODO copy data to MAV
        
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
        return this->get_override("pyReadBlock");
    };
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

void export_adapters() {
    exposeSource<3,vigra::UInt8>("Source3U8");
}
