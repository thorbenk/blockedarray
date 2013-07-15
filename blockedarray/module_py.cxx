#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API

#include "Python.h" 

#include <boost/python.hpp>

#include <vigra/numpy_array.hxx>

#include "blockedarray_py.h"

BOOST_PYTHON_MODULE_INIT(_blockedarray) {
    _import_array();
    vigra::import_vigranumpy();
    boost::python::numeric::array::set_module_and_type("numpy", "ndarray");

    export_blockedArray();
}  