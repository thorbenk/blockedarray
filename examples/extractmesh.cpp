#include <vigra/tinyvector.hxx>

#include <bw/meshextractor.h>
#include <bw/sourcehdf5.h>
#include <bw/roi.h>

int main(int argc, char **argv) {
    using namespace BW;
    typedef vigra::TinyVector<int, 3> V;
    
    if(argc != 4) {
        std::cout << "usage: ./extractmesh hdf5file hdf5group meshfile" << std::endl;
        return 0;
    }
    std::string hdf5file(argv[1]);
    std::string hdf5group(argv[2]);
    std::string meshfile(argv[3]);

    const int N = 32;
    V blockShape(N,N,N);
    SourceHDF5<3, uint32_t> source(hdf5file, hdf5group);
    std::cout << source.shape() << std::endl;

    MeshExtractor<3, uint32_t> me(&source, blockShape);
    me.run(1, meshfile);
    
    return 0;
}
