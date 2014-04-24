#include <vigra/tinyvector.hxx>

#include <bw/resampleimage.h>
#include <bw/sourcehdf5.h>
#include <bw/sinkhdf5.h>
#include <bw/roi.h>

class H5DataDescriptor {
public:
    static H5DataDescriptor fromString(const std::string& s);
    std::string filename;
    std::string groupname;
};

std::ostream& operator<<(std::ostream& o, const H5DataDescriptor d)
{
    o << d.filename << "/" << d.groupname;
    return o;
}

H5DataDescriptor H5DataDescriptor::fromString(const std::string& s)
{
    size_t pos = s.find_last_of("/");
    
    H5DataDescriptor d;
    d.filename  = s.substr(0, pos);
    d.groupname = s.substr(pos+1, s.size()); 
    return d;
}

int main(int argc, char **argv) {
    using namespace BW;
    typedef vigra::TinyVector<int, 3> V;
    
    if(argc != 3) {
        std::cout << "usage: ./extractmesh hdf5file_in hdf5file_out " << std::endl;
        return 0;
    }
    H5DataDescriptor in  = H5DataDescriptor::fromString(argv[1]);
    H5DataDescriptor out = H5DataDescriptor::fromString(argv[2]);
    
    double factor = 1.0/4.0;

    V outBlockShape(32,32,32); 
    V inBlockShape = factor < 1.0 ? vigra::floor(1.0/factor*outBlockShape) : vigra::ceil(1.0/factor*inBlockShape);
    
    std::cout << "inBlockShape  = " << inBlockShape << std::endl;
    std::cout << "outBlockShape = " << outBlockShape << std::endl;
    
    std::cout << "in  = " << in << std::endl;
    std::cout << "out = " << out << std::endl;
    
    SourceHDF5<3, uint32_t> source(in.filename, in.groupname);
    std::cout << source.shape() << std::endl;

    ResampleImage<3, uint32_t> r(&source, inBlockShape);
    
    SinkHDF5<3, uint32_t> sink(out.filename, out.groupname, 0 /* no compression */);
    
    r.run(1.0/4.0, &sink, outBlockShape);
    
    return 0;
}