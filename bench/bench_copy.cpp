#include <Python.h>

#include <map>
#include <vector>
#include <algorithm>
#include <numeric>

#include <vigra/timing.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/numpy_array.hxx>

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/concept_check.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include "test_utils.h"

template<typename T>
double bench_new(size_t size) {
    USETICTOC;
    double t = 0.0;
    TIC;
    T* d = new T[size];
    delete[] d;
    return TOCN;
}

template<typename T>
double bench_allocator_fill(size_t size) {
    //TODO:
    //
    //read http://stackoverflow.com/questions/6047840/does-an-allocator-construct-loop-equal-stduninitialized-copy
    //
    
    USETICTOC;
    std::allocator<T> alloc; 
    
    T* ptr = alloc.allocate(size);
    const T zero = 0;
    const T& zeroRef = zero;
    const vigra::MultiArrayIndex s = size;
    double t = 0.0;
    try {
        TIC; 
        for (vigra::MultiArrayIndex i = 0; i < s; ++i) {
            alloc.construct (ptr + i, zeroRef /*init*/);
        }
        t = TOCN;
    }
    catch (...) {
        for(size_t j = 0; j < size; ++j) {
            alloc.destroy (ptr + j);
        }
        alloc.deallocate(ptr, size);
        throw;
    }
    alloc.deallocate(ptr, size);
    return t;
}

template<typename T>
double bench_writeasc(size_t size) {
    USETICTOC;
    double t = 0.0;
    T* d = new T[size];
    FillRandom<T, T*>::fillRandom(d, d+size);
    TIC;
    for(size_t j=0; j<size; ++j) {
        d[j] = j;
    }
    t = TOCN;
    delete[] d;
    return t;
}

template<typename T>
double bench_fill(size_t size) {
    USETICTOC;
    double t = 0.0;
    T* d = new T[size];
    FillRandom<T, T*>::fillRandom(d, d+size);
    
    TIC;
    std::fill(d, d+size, 0);
    t = TOCN;
    
    delete[] d;
    return t; 
}

template<typename T>
double bench_memset(size_t size) {
    USETICTOC;
    T* d = new T[size];
    FillRandom<T, T*>::fillRandom(d, d+size);
    
    double t = 0.0;
    TIC;
    memset(d, 0, size*sizeof(T));
    t = TOCN;
    
    delete[] d;
    return t;
}

template<typename T>
double bench_stdcopy(size_t size) {
    USETICTOC;
    double t = 0.0;
    T* d1  = new T[size];
    T* d2 = new T[size];
    FillRandom<T, T*>::fillRandom(d1, d1+size);
    FillRandom<T, T*>::fillRandom(d2, d2+size);
    
    TIC;
    std::copy(d1, d1+size, d2);
    t = TOCN;
    
    delete[] d1;
    delete[] d2;
    return t;
}

template<typename T>
double bench_memcpy(size_t size) {
    USETICTOC;
    double t = 0.0;
    T* d1 = new T[size];
    T* d2 = new T[size];
    FillRandom<T, T*>::fillRandom(d1, d1+size);
    FillRandom<T, T*>::fillRandom(d2, d2+size);
    
    TIC;
    memcpy(d2, d1, size*sizeof(T));
    t = TOCN;
    
    delete[] d1;
    delete[] d2;
    return t;
}

template<int N, class T>
double bench_MultiArray_new(
    typename vigra::MultiArrayShape<N>::type shape
) {
    USETICTOC;
    double t;
    TIC;
    vigra::MultiArray<N,T> a(shape);
    t = TOCN;
    
    FillRandom<T, T*>::fillRandom(&a[0], &a[0]+a.size());
    return t;
}

template<int N, class T>
double bench_MultiArray_writeasc(
    typename vigra::MultiArrayShape<N>::type shape
) {
    USETICTOC;
    vigra::MultiArray<N,T> a(shape);
    FillRandom<T, T*>::fillRandom(&a[0], &a[0]+a.size());
    
    double t = 0.0;
    TIC;
    for(size_t j=0; j<a.size(); ++j) {
        a(j) = j; //ATTENTION: this is much faster than a[j], but still slow
    }
    t = TOCN;
    
    return t;
}

template<int N, class T>
double bench_MultiArray_copy_assignment(
    typename vigra::MultiArrayShape<N>::type shape
) {
    USETICTOC;
    vigra::MultiArray<N,T> a(shape);
    FillRandom<T, T*>::fillRandom(&a[0], &a[0]+a.size());
    vigra::MultiArray<N,T> b(a.shape());
    FillRandom<T, T*>::fillRandom(&b[0], &b[0]+b.size());
    
    double t = 0.0;
    TIC;
    b = a;
    t = TOCN;
    
    return t;
}

template<int N, class T>
double bench_MultiArray_copy_view(
    typename vigra::MultiArrayShape<N>::type shape
) {
    USETICTOC;
    double t;
    vigra::MultiArray<N,T> a(shape);
    FillRandom<T, T*>::fillRandom(&a[0], &a[0]+a.size());
    vigra::MultiArray<N,T> b(shape);
    FillRandom<T, T*>::fillRandom(&b[0], &b[0]+b.size());
    vigra::MultiArrayView<N,T> bView = b.view();
    
    TIC;
    b = a.subarray(typename vigra::MultiArrayShape<N>::type(),shape);
    return TOCN;
}

class Benchmark {
    public:
    Benchmark() {}
    Benchmark(std::string group, std::string name, boost::function<double()> bmark)
        : group_(group)
        , name_(name)
        , bmark_(bmark)
        {}
    std::string name() const { return name_; }
    void run() { timings_.push_back(bmark_()); }
    
    double avg() { return std::accumulate(timings_.begin(), timings_.end(), 0.0)/double(timings_.size()); }
    double std() {
        double mean = avg();
        std::vector<double> zeroMean(timings_);
        std::transform(zeroMean.begin(), zeroMean.end(), zeroMean.begin(), std::bind2nd(std::minus<float>(), mean));
        double deviation = std::inner_product(zeroMean.begin(),zeroMean.end(), zeroMean.begin(), 0.0);
        return std::sqrt(deviation/double(timings_.size()-1));
    }
    
    private:
    std::string group_;
    std::string name_;
    boost::function<double()> bmark_; 
    std::vector<double> timings_;
};

class Benchmarks {
    public:
    typedef std::map<std::string, std::vector<int> > Groups;
    
    Benchmarks() : size_(0), repetitions_(10) {}
    
    void setRepetitions(int r) { repetitions_ = r; }
        
    void add(std::string group, std::string name, boost::function<double()> bmark) {
        bmarks_.push_back(Benchmark(group, name, bmark));
        groups_[group].push_back(size_);
        ++size_;
    }
    
    void run() {
        std::vector<int> x(size_);
        for(int i=0; i<x.size(); ++i) { x[i] = i; }
        
        for(int n=0; n<repetitions_; ++n) {
            std::random_shuffle(x.begin(), x.end());
            int m=0;
            for(std::vector<int>::iterator i=x.begin(); i!=x.end(); ++i, ++m) {
                std::cout << "repetition " << n << "/" << repetitions_
                          << " : benchmark " << m << "/" << size_ << " "
                          << "    \r" << std::flush;
                Benchmark& bm = bmarks_[*i];
                bm.run();
            }
        }
        std::cout << std::endl << std::endl;
        
        size_t w = 0;
        BOOST_FOREACH(const Benchmark& bm, bmarks_) {
            w = std::max(w, bm.name().size());
        }
        
        for(Groups::const_iterator i=groups_.begin();
            i!=groups_.end(); ++i)
        {
            std::cout << "* " << i->first << std::endl;
            std::cout << "  " << std::string(i->first.size(), '-') << std::endl;
            std::cout << std::endl;
            
            for(std::vector<int>::const_iterator j=i->second.begin();
                j != i->second.end(); ++j)
            {
                Benchmark& bm = bmarks_[*j];
                std::cout << "  " << bm.name()
                          << std::string(w-bm.name().size(), ' ')
                          << " : "
                          << std::setw(6) << std::fixed << std::setprecision(2) << bm.avg()
                          << " +- "
                          << std::setw(6) << std::fixed << std::setprecision(2) << bm.std() << std::endl;
            }
            std::cout << std::endl << std::endl;
        }
    }
    
    private:
    std::vector<Benchmark> bmarks_;
    std::map<std::string, std::vector<int> > groups_;
    int size_;
    int repetitions_;
};

template<class T>
void addBenchmarks(Benchmarks& b, const std::string& t) {
    const int N = 5;
    
    vigra::Shape5 sh(1,300,310,320,1);
    size_t size = 1; for(int i=0; i<N; ++i) { size *= sh[i]; };
    std::cout << "*** size " << size*sizeof(T)/(1024*1024) << "MB:" << std::endl;
    
    using boost::bind;
    using boost::format;
    
    b.setRepetitions(5);
    
    b.add("new", "new", bind(bench_new<T>, size));
    
    b.add((format("initialize with 0 <%s>") % t).str(), "std::fill",      bind(bench_fill<T>, size));
    b.add((format("initialize with 0 <%s>") % t).str(), "allocator fill", bind(bench_allocator_fill<T>, size));
    b.add((format("initialize with 0 <%s>") % t).str(), "std::memset",    bind(bench_memset<T>, size));
    b.add((format("initialize with 0 <%s>") % t).str(), "MultiArray()",   bind(bench_MultiArray_new<N,T>, sh));
          
    b.add((format("full copy <%s>") % t).str(), "std::copy", bind(bench_stdcopy<T>, size));
    b.add((format("full copy <%s>") % t).str(), "std::memcpy", bind(bench_memcpy<T>, size));
    b.add((format("full copy <%s>") % t).str(), "MultiArray::operator=", bind(bench_MultiArray_copy_assignment<N,T>, sh));
    b.add((format("full copy <%s>") % t).str(), "MultiArrayView::operator=", bind(bench_MultiArray_copy_view<N,T>, sh));
          
    b.add((format("write ascending numbers <%s>") % t).str(), "T* ascending", bind(bench_writeasc<T>, size));
    b.add((format("write ascending numbers <%s>") % t).str(), "MultiArray ascending", bind(bench_MultiArray_writeasc<N, T>, sh));
}

int main() {
    using namespace vigra;
    using std::cout; using std::endl;
    USETICTOC;
    
    const int N = 5;
    typedef unsigned int T;
    
    Shape5 sh(1,300,310,320,1);
    Shape5 shR(1,320,310,300,1);
    
    size_t size = 1; for(int i=0; i<N; ++i) { size *= sh[i]; };
    std::cout << "*** size " << size*sizeof(T)/(1024*1024) << "MB:" << std::endl;
    
    using boost::bind;
    Benchmarks b;
    
    addBenchmarks<unsigned char>(b, "uint8");
    addBenchmarks<vigra::UInt32> (b, "uint32");
    addBenchmarks<float> (b, "float");
    addBenchmarks<double> (b, "double");
    
    b.setRepetitions(5);
    b.run();
        
    MultiArray<N,T> dataCpp(sh);
    for(int i=0; i<dataCpp.size(); ++i) {
        dataCpp(i) = i;
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
    pyCode << "), dtype=numpy.uint32); ";
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
    
    unsigned int* pyData = reinterpret_cast<T*>(PyArray_DATA(pyA));
    
    for(int i=0; i<dataCpp.size(); ++i) {
        if(*(&dataCpp[0]+i) != *(pyData+i)) {
            throw std::runtime_error("bad");
        }
    }
   
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
    