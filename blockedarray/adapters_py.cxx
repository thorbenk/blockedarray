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
    
    
    V getShape() const
    {
        return this->shape_;
    }
    
    void setShape(V shape)
    {
        this->shape_ = shape;
    }

    V getBlockShape() const
    {
        return this->blockShape_;
    }
    
    void setBlockShape(V shape)
    {
        this->blockShape_ = shape;
    }
};



template<int N, class T>
void exportSpecificSourceAdapter(std::string suffix) {
    using namespace boost::python;
    
    class_<PySourceABC<N,T>, boost::noncopyable>(("PySource" + suffix).c_str())
        .def("pySetRoi", pure_virtual(&PySourceABC<N,T>::pySetRoi))
        .def("pyShape", pure_virtual(&PySourceABC<N,T>::pyShape))
        .def("pyReadBlock", pure_virtual(&PySourceABC<N,T>::pyReadBlock))
    ;
        
    class_<PySinkABC<N,T>, boost::noncopyable>(("PySink" + suffix).c_str())
        .def("pyWriteBlock", pure_virtual(&PySinkABC<N,T>::pyWriteBlock))
        .add_property("shape", &PySinkABC<N,T>::getShape, &PySinkABC<N,T>::setShape)
        .add_property("blockShape", &PySinkABC<N,T>::getBlockShape, &PySinkABC<N,T>::setBlockShape)
    ;
}

template <int N>
void exportSourceAdaptersForDim()
{
    
    exportSpecificSourceAdapter<N,vigra::UInt8>("U8");
    //exportSpecificSourceAdapter<N,vigra::UInt16>("U16");
    exportSpecificSourceAdapter<N,vigra::UInt32>("U32");
    
    exportSpecificSourceAdapter<N,vigra::Int8>("S8");
    //exportSpecificSourceAdapter<N,vigra::Int16>("S16");
    exportSpecificSourceAdapter<N,vigra::Int32>("S32");
    
    exportSpecificSourceAdapter<N,float>("F");
    exportSpecificSourceAdapter<N,double>("D");
}

template void exportSourceAdaptersForDim<2>();
template void exportSourceAdaptersForDim<3>();

