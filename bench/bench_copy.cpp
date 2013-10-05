#include <vigra/timing.hxx>
#include <vigra/multi_array.hxx>

int main() {
    using namespace vigra;
    
    Shape5 sh(1,300,300,300,1);
    
    MultiArray<5,unsigned char> a(sh);
    
    USETICTOC;
    TIC;
    MultiArray<5,unsigned char> b = a;
    std::cout << "copy via assignment op: " << TOCS << std::endl;
    
    TIC;
    MultiArray<5, unsigned char> out(sh);
    MultiArrayView<5, unsigned char> outView = out.view();
    std::cout << "new: " << TOCS << std::endl;
   
    TIC;
    MultiArrayView<5,unsigned char> mydata(sh, &a[0]);
    out = mydata.subarray(Shape5(),sh);
    std::cout << "copy via view and subarray: " << TOCS << std::endl;
    
    TIC;
    outView = mydata.subarray(Shape5(),sh);
    std::cout << "copy via view, view and subarray: " << TOCS << std::endl;
    
    return 0;
}
    