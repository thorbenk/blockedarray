/************************************************************************/
/*                                                                      */
/*    Copyright 2013 by Thorben Kroeger                                 */
/*    thorben.kroeger@iwr.uni-heidelberg.de                             */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

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

template <int N, class T>
void exportSpecificSource(std::string suffix)
{
    
    using namespace boost::python;
    const char *source = ("Source"+suffix).c_str();
    const char *sink = ("Sink"+suffix).c_str();
    class_<Source<N, T> >(source, no_init);
    class_<Sink<N, T> >(sink, no_init);
}

template <int N>
void exportSourceForDim()
{
    exportSpecificSource<N,vigra::UInt8>("U8");
    //exportSpecificSource<N,vigra::UInt16>("U16");
    exportSpecificSource<N,vigra::UInt32>("U32");

    exportSpecificSource<N,vigra::Int8>("S8");
    //exportSpecificSource<N,vigra::Int16>("S16");
    exportSpecificSource<N,vigra::Int32>("S32");

    exportSpecificSource<N,float>("F");
    exportSpecificSource<N,double>("D");
}


template <int N>
void exportAllForDim()
{
    
    using namespace boost::python;
    
    // set the correct module
    std::stringstream n; n << N;
    
    std::stringstream fullModname; fullModname << "_blockedarray.dim" << N;
    std::stringstream modName; modName << "dim" << N;
    
    //see: http://isolation-nation.blogspot.de/2008/09/packages-in-python-extension-modules.html
    object module(handle<>(borrowed(PyImport_AddModule(fullModname.str().c_str()))));
    scope().attr(modName.str().c_str()) = module;
    scope s = module;
    
    
    // export source and sink
    exportSourceForDim<N>();
    
    
    // connected components class
    typedef ConnectedComponents<N> BCC;
    class_<BCC>("ConnectedComponents",
                init<Source<N, vigra::UInt8>*, typename BCC::V>())
    .def("writeResult", &BCC::writeResult,
         (arg("hdf5file"), arg("hdf5group"), arg("compression")=1))
    .def("writeToSink", &BCC::writeToSink,
         (arg("sink")))
    ;
    
    
    // ROI
    typedef typename Roi<N>::V V;
    
    class_< Roi<N> >("Roi", init<V,V>())
    .def_readonly("p", &Roi<N>::p)
    .def_readonly("q", &Roi<N>::p)
    ;
    
}


void export_blockwiseCC() 
{
    exportAllForDim<2>();
    exportAllForDim<3>();
}
