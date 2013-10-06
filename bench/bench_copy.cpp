#include <Python.h>
#include <vigra/timing.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/numpy_array.hxx>
#include <boost/python.hpp>

int main() {
    using namespace vigra;
    using std::cout; using std::endl;
    USETICTOC;
    
    const int N = 5;
    typedef unsigned char T;
    
    Shape5 sh(1,300,310,320,1);
    
    size_t size = 1; for(int i=0; i<N; ++i) { size *= sh[i]; };
   
    {
        TIC;
        T* d = new T[size];
        double t = TOCN;
        std::cout << "size " << size*sizeof(T)/(1024*1024) << "MB:" << std::endl;
        std::cout << "  new:      " << t << " msec." << std::endl;
       
        for(int i=0; i<5; ++i) {
            TIC;
            std::fill(d, d+size, 0);
            t = TOCN;
            std::cout << "  fill:     " << t << " msec." << std::endl;
            
            TIC;
            memset(d, 0, size*sizeof(T));
            t = TOCN;
            std::cout << "  memset:   " << t << " msec." << std::endl;
        }
        
        T* d2 = new T[size];
        TIC;
        std::copy(d, d+size, d2);
        t = TOCN;
        std::cout << "  std::copy " << t << " msec. " << std::endl;
        
        delete[] d;
        delete[] d2;
    }
    
    MultiArray<N,T> dataCpp(sh);
    typedef MultiArray<N,T>::view_type view_type;
    
    TIC;
    MultiArray<N,T> b = dataCpp;
    std::cout << "copy via assignment op: " << TOCS << std::endl;
    
    TIC;
    MultiArray<N, T> out(sh);
    MultiArrayView<N, T> outView = out.view();
    std::cout << "new: " << TOCS << std::endl;
   
    TIC;
    MultiArrayView<N,T> mydata(sh, &dataCpp[0]);
    out = mydata.subarray(Shape5(),sh);
    std::cout << "copy via view and subarray: " << TOCS << std::endl;
    
    TIC;
    outView = mydata.subarray(Shape5(),sh);
    std::cout << "copy via view, view and subarray: " << TOCS << std::endl;
   
    /*
    std::cout << "data has strides: " << a.stride() << std::endl;
    std::cout << "out has strides:  " << out.stride() << std::endl;
    MultiArrayView<N, T> outViewP = out.view();
    outViewP = outViewP.permuteStridesAscending();
    std::cout << "outViewP has strides:  " << outViewP.stride() << ", " << outViewP.strideOrdering() << std::endl;
    outViewP = outViewP.permuteStridesDescending();
    std::cout << "outViewP has strides:  " << outViewP.stride() << ", " << outViewP.strideOrdering() << std::endl;
    TIC;
    outViewP = mydata.subarray(ShapeN(),sh);
    std::cout << "strides differ " << TOCS << std::endl;
    */

    using namespace boost::python;
    Py_Initialize();
    _import_array();
    object main_module((handle<>(borrowed(PyImport_AddModule("__main__")))));
    object main_namespace = main_module.attr("__dict__");
    
    std::stringstream pyCode;
    pyCode << "import numpy; ";
    pyCode << "a = numpy.zeros((";
    for(int i=0; i<N; ++i) { pyCode << dataCpp.shape(i); if(i<N-1) { pyCode << ","; } }
    pyCode << "), dtype=numpy.uint8);";
    std::cout << "running python code \"" << pyCode.str() << "\"" << std::endl;
    
    handle<> ignored((PyRun_String(pyCode.str().c_str(), 
                      Py_file_input, main_namespace.ptr(), main_namespace.ptr() ) ));
    
    object objectA = main_namespace["a"]; 
    if(!PyArray_Check(objectA.ptr())) { throw std::runtime_error("failed to get array"); }
    PyArrayObject* pyA = (PyArrayObject*)objectA.ptr();
    std::cout << "numpy array dim     = " << PyArray_NDIM(pyA) << std::endl;
    std::cout << "            shape   = ";
    std::copy(PyArray_DIMS(pyA), PyArray_DIMS(pyA)+PyArray_NDIM(pyA), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    std::cout << "            strides = ";
    std::copy(PyArray_STRIDES(pyA), PyArray_STRIDES(pyA)+PyArray_NDIM(pyA), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    vigra::NumpyArray<N,T> numpyA((PyObject*)pyA);
    std::cout << "vigra::NumpyArray strides = " << numpyA.stride() << std::endl;
    
    TIC;
    numpyA = dataCpp;
    std::cout << "copy from c++ into Numpy array (1): " << TOCS << std::endl;
    
    MultiArray<N,T> dataCppR(Shape5(1,320,310,300,1));
    std::cout << "dataCppR strides = " << dataCppR.stride() << std::endl;
    TIC;
    numpyA = dataCppR.permuteStridesDescending();
    std::cout << "copy from c++ into Numpy array (2):" << TOCS << std::endl;
    
    return 0;
}
    