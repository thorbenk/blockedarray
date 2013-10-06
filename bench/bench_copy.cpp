#include <Python.h>
#include <vigra/timing.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/numpy_array.hxx>
#include <boost/python.hpp>

void thrashCache() {
    char* d  = new char[1024*1024*50];
    char* d2 = new char[1024*1024*50];
    std::copy(d, d+1024*1024*50, d2);
    char* d3 = new char[1024*1024*50];
    std::copy(d2, d2+1024*1024*50, d3);
    
    delete[] d;
    delete[] d2;
    delete[] d3;
}

template<typename T>
void bench_new(size_t size, size_t testIter) {
    USETICTOC;
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        TIC;
        T* d = new T[size];
        t += TOCN;
        delete[] d;
    }
    std::cout << "  new:      " << t/testIter << " msec." << std::endl;
}

template<typename T>
void bench_allocator_fill(size_t size, size_t testIter) {
    //TODO:
    //
    //read http://stackoverflow.com/questions/6047840/does-an-allocator-construct-loop-equal-stduninitialized-copy
    //
    
    USETICTOC;
    std::allocator<T> alloc; 
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        T* ptr = alloc.allocate(size);
        const T zero = 0;
        const T& zeroRef = zero;
        const vigra::MultiArrayIndex s = size;
        try {
            TIC; 
            for (vigra::MultiArrayIndex i = 0; i < s; ++i) {
                alloc.construct (ptr + i, zeroRef /*init*/);
            }
            t += TOCN;
        }
        catch (...) {
            for(size_t j = 0; j < size; ++j) {
                alloc.destroy (ptr + j);
            }
            alloc.deallocate(ptr, size);
            throw;
        }
        alloc.deallocate(ptr, size);
    }
    std::cout << "  alloc fill:  " << t/testIter << " msec." << std::endl;
}

template<typename T>
void bench_writeasc(size_t size, size_t testIter) {
    USETICTOC;
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        T* d = new T[size];
        TIC;
        for(size_t j=0; j<size; ++j) {
            d[j] = j;
        }
        t += TOCN;
        delete[] d;
    }
    std::cout << "  write asc:           " << t/testIter << " msec." << std::endl;
}

template<typename T>
void bench_fill(size_t size, size_t testIter) {
    USETICTOC;
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        T* d = new T[size];
        for(int i=0; i<size; ++i) { d[i] = i; }
        TIC;
        std::fill(d, d+size, 0);
        t += TOCN;
        delete[] d;
    }
    std::cout << "  fill:        " << t/testIter << " msec." << std::endl;
}

template<typename T>
void bench_memset(size_t size, size_t testIter) {
    USETICTOC;
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        T* d = new T[size];
        for(int i=0; i<size; ++i) { d[i] = i; }
        TIC;
        memset(d, 0, size*sizeof(T));
        t += TOCN;
        delete[] d;
    }
    std::cout << "  memset:      " << t/testIter << " msec." << std::endl;
}

template<typename T>
void bench_stdcopy(size_t size, size_t testIter) {
    USETICTOC;
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        T* d  = new T[size];
        T* d2 = new T[size];
        TIC;
        std::copy(d, d+size, d2);
        t += TOCN;
        delete[] d;
        delete[] d2;
    }
    std::cout << "  std::copy                 " << t/testIter << " msec. " << std::endl;
}

template<typename T>
void bench_memcpy(size_t size, size_t testIter) {
    USETICTOC;
    double t = 0.0;
    for(int tI=0; tI<testIter; ++tI) {
        T* d  = new T[size];
        T* d2 = new T[size];
        TIC;
        memcpy(d2, d, size*sizeof(T));
        t += TOCN;
        delete[] d;
        delete[] d2;
    }
    std::cout << "  memcpy                    " << t/testIter << " msec. " << std::endl;
}

template<int N, class T>
void bench_MultiArray_new(
    typename vigra::MultiArrayShape<N>::type shape,
    size_t testIter
) {
    USETICTOC;
    double t;
    for(int tI=0; tI<testIter; ++tI) {
        TIC;
        vigra::MultiArray<N,T> a(shape);
        t += TOCN;
    }
    std::cout << "  MultiArray() " << t/testIter << " msec. " << std::endl;
}

template<int N, class T>
void bench_MultiArray_writeasc(
    typename vigra::MultiArrayShape<N>::type shape,
    size_t testIter
) {
    USETICTOC;
    double t;
    for(int tI=0; tI<testIter; ++tI) {
        vigra::MultiArray<N,T> a(shape);
        TIC;
        for(size_t j=0; j<a.size(); ++j) {
            a(j) = j; //ATTENTION: this is much faster than a[j], but still slow
        }
        t += TOCN;
    }
    std::cout << "  MultiArray write asc " << t/testIter << " msec. " << std::endl;
}

template<int N, class T>
void bench_MultiArray_copy_assignment(
    typename vigra::MultiArrayShape<N>::type shape,
    size_t testIter
) {
    USETICTOC;
    double t;
    for(int tI=0; tI<testIter; ++tI) {
        vigra::MultiArray<N,T> a(shape);
        //for(int i=0; i<a.size(); ++i) { a[i] = i; }
        vigra::MultiArray<N,T> b(a.shape());
        TIC;
        b = a;
        t += TOCN;
    }
    std::cout << "  MultiArray::operator=     " << t/testIter << " msec. " << std::endl;
}

template<int N, class T>
void bench_MultiArray_copy_view(
    typename vigra::MultiArrayShape<N>::type shape,
    size_t testIter
) {
    USETICTOC;
    double t;
    for(int tI=0; tI<testIter; ++tI) {
        vigra::MultiArray<N,T> a(shape);
        vigra::MultiArray<N,T> b(shape);
        vigra::MultiArrayView<N,T> bView = b.view();
        TIC;
        b = a.subarray(typename vigra::MultiArrayShape<N>::type(),shape);
        t += TOCN;
    }
    std::cout << "  MultiArrayView::operator= " << t/testIter << " msec." << std::endl;
}

int main() {
    using namespace vigra;
    using std::cout; using std::endl;
    USETICTOC;
    
    const int N = 5;
    typedef unsigned char T;
    
    Shape5 sh(1,300,310,320,1);
    
    size_t size = 1; for(int i=0; i<N; ++i) { size *= sh[i]; };
    std::cout << "*** size " << size*sizeof(T)/(1024*1024) << "MB:" << std::endl;
    
    const int testIter = 20;
   
    std::cout << "* new" << std::endl;
    bench_new<T>(size, testIter);
    std::cout << std::endl;
    
    std::cout << "* initialize with 0" << std::endl;
    bench_fill<T>(size, testIter);
    bench_allocator_fill<T>(size, testIter);
    bench_memset<T>(size, testIter);
    bench_MultiArray_new<N,T>(sh, testIter);
    std::cout << std::endl;
    
    std::cout << "* full copy" << std::endl;
    bench_stdcopy<T>(size, testIter);
    bench_memcpy<T>(size, testIter);
    bench_MultiArray_copy_assignment<N,T>(sh, testIter);
    bench_MultiArray_copy_view<N,T>(sh, testIter);
    std::cout << std::endl;
    
    std::cout << "* write ascending numbers" << std::endl;
    bench_writeasc<T>(size, testIter);
    bench_MultiArray_writeasc<N,T>(sh, testIter);
    std::cout << std::endl;
        
    MultiArray<N,T> dataCpp(sh);
    for(int i=0; i<dataCpp.size(); ++i) {
        *(&dataCpp[0]+i) = i;
    }
    typedef MultiArray<N,T>::view_type view_type;
    
    using namespace boost::python;
    Py_Initialize();
    _import_array();
    object main_module((handle<>(borrowed(PyImport_AddModule("__main__")))));
    object main_namespace = main_module.attr("__dict__");
    
    std::stringstream pyCode;
    pyCode << "import numpy; ";
    pyCode << "a = numpy.zeros((";
    for(int i=0; i<N; ++i) { pyCode << dataCpp.shape(i); if(i<N-1) { pyCode << ","; } }
    pyCode << "), dtype=numpy.uint8); ";
    pyCode << "a[:] = numpy.arange(0,"<< size << ").reshape(";
    for(int i=0; i<N; ++i) { pyCode << dataCpp.shape(i); if(i<N-1) { pyCode << ","; } }
    pyCode << ");";
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
    
    T* pyData = reinterpret_cast<T*>(PyArray_DATA(pyA));
   
    std::cout << "C++ data: ";
    std::cout << (int)dataCpp(0,0,0,0,0) << ", ";
    std::cout << (int)dataCpp(0,1,0,0,0) << ", ";
    std::cout << (int)dataCpp(0,2,0,0,0) <<  " | strides = " << dataCpp.stride() << std::endl;
    
    std::cout << "py  data: " << (int)*(pyData) << ", " << (int)*(pyData+1) << ", " << (int)*(pyData+2) << " | strides = (";
    std::copy(PyArray_STRIDES(pyA), PyArray_STRIDES(pyA)+PyArray_NDIM(pyA), std::ostream_iterator<int>(std::cout, ", "));
    std::cout << ")" << std::endl;
    
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
    