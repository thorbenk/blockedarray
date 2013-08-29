#ifndef BW_SOURCEKNOSSOS_H
#define BW_SOURCEKNOSSOS_H

#include <fstream>
#include <numeric>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <vigra/hdf5impex.hxx>

#include <bw/source.h>
#include <bw/blocking.h>

namespace BW {
    
/**
 * Read a block of data from a HDF5File
 * 
 * This code is based on:
 * https://github.com/ilastik/knossos_impex
 */
template<int N, class T>
class SourceKnossos : public Source<N,T> {
    public:
    typedef typename Source<N,T>::V V;
    
    SourceKnossos(const std::string& directory)
        : Source<N,T>()
        , directory_(directory)
    {
        std::ifstream f((directory+"/knossos.conf").c_str());
        std::string line;
        while(std::getline(f, line)) {
            boost::trim(line);
            std::vector<std::string> tokens;
            boost::split(tokens, line, boost::is_any_of(" ;"), boost::token_compress_on);
            if(tokens[0] == "experiment" && tokens[1] == "name") {
                experimentName_ = tokens[2];
                boost::replace_all(experimentName_, "\"", "");
            }
            else if(tokens[0] == "scale") {
                std::string num = tokens[2];
                boost::replace_all(num, ";", "");
                double s = boost::lexical_cast<double>(num);
                if(tokens[1] == "x") scale_[0] = s;
                else if(tokens[1] == "y") scale_[1] = s;
                else if(tokens[1] == "z") scale_[2] = s;
            }
            else if(tokens[0] == "boundary") {
                std::string num = tokens[2];
                boost::replace_all(num, ";", "");
                double s = boost::lexical_cast<int>(num);
                if(tokens[1] == "x") boundary_[0] = s;
                else if(tokens[1] == "y") boundary_[1] = s;
                else if(tokens[1] == "z") boundary_[2] = s;
            }
            else if(tokens[0] == "magnification") {
                magnification_ = boost::lexical_cast<int>(tokens[1]);
            }
        }
        std::cout << "experiment name: " << experimentName_ << std::endl;
        std::cout << "scale :          "; std::copy(scale_, scale_+3, std::ostream_iterator<double>(std::cout, " ")); std::cout << std::endl;
        std::cout << "boundary :       "; std::copy(boundary_, boundary_+3, std::ostream_iterator<int>(std::cout, " ")); std::cout << std::endl;
        std::cout << "magnification:   " << magnification_ << std::endl;
        
        Blocking<3> bb(Roi<3>(V(0,0,0), V(boundary_[0], boundary_[1], boundary_[2])), V(128,128,128));
        
        dataRoi_ = Roi<3>(V(0,0,0), V(boundary_[0], boundary_[1], boundary_[2]));
    }
    
    virtual void setRoi(Roi<N> roi) {
       roi_ = roi; 
    }
    
    virtual V shape() const {
        using namespace vigra;
        
        return V(); 
    }
    
    virtual bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const {
        vigra_precondition(roi.shape() == block.shape(), "shapes differ");
        
        std::cout << "* reading block " << roi << std::endl;
        
        V startBlock, endBlock;
        for(int i=0; i<3; ++i) {
            startBlock[i] = roi.p[i] / 128;
            endBlock[i]   = (roi.q[i]-1) / 128 + 1;
        }
       
        size_t nBlocks = 1;
        for(int i=0; i<3; ++i) nBlocks *= endBlock[i]-startBlock[i];
        
        std::cout << "  loading blocks " << startBlock << " until " << endBlock << std::endl;
       
        int counter = 0;
        for(int x=startBlock[0]; x<endBlock[0]; ++x) {
        for(int y=startBlock[1]; y<endBlock[1]; ++y) {
        for(int z=startBlock[2]; z<endBlock[2]; ++z) {
            ++counter;
            
            V coor(x,y,z);
            
            std::cout << "  * loading block " << counter << "/" << nBlocks << " | " << x << ", " << y << ", " << z << std::endl;
            
            V p;
            V offset;
            V q(128,128,128);
            for(int i=0; i<3; ++i) {
                if(startBlock[i] == coor[i])
                    p[i]          = roi.p[i] - 128*coor[i];
                else
                    offset[i] = 128*(coor[i]-startBlock[i]-1); 
                if(endBlock[i] == coor[i]+1)
                    q[i]          = roi.q[i] % 128 + p[i];
            }
            V bP = p + offset;
            V bQ = q + offset;
            
            //std::cout << "offset=" << offset << std::endl;
            //std::cout << "bP=" << bP << ", bQ=" << bQ << " | " << block.shape(0) << ", " << block.shape(1) << ", " << block.shape(2) << std::endl;
            //std::cout << "p=" << p << ", q=" << q << std::endl;
            
            Roi<3> blockRoiFull(128*coor, 128*(V(1,1,1)+coor));
            Roi<3> blockRoi;
            blockRoiFull.intersect(dataRoi_, blockRoi);
            
            std::string blockFile = fileForBlockCoor(coor);
            std::cout << "    file " << blockFile << std::endl;

            std::ifstream f(blockFile.c_str(), std::ios::binary | std::ios::ate);
           
            if(f.tellg() != 128*128*128) {
                std::stringstream err;
                err << "Error while reading '" << blockFile << "':" << std::endl;
                err << "Was expecting a file of size 128^3 bytes length," << std::endl;
                err << "but file has length " << f.tellg();
                throw std::runtime_error(err.str());
            }
           
            vigra::MultiArray<3, unsigned char> b(vigra::Shape3(128,128,128));
            f.seekg(0);
            f.read(reinterpret_cast<char*>(b.data()), 128*128*128);
            f.close();
           
            block.subarray(bP, bQ).copy(b.subarray(p, q));
        }
        }
        }
        
        return true;
    }
    
    private:
        
    std::string fileForBlockCoor(V coor) const {
        int x = coor[0];
        int y = coor[1];
        int z = coor[2];
        std::stringstream path;
        path << directory_ << "/";
        path << "x" << std::setw(4) << std::setfill('0') << x << "/";
        path << "y" << std::setw(4) << std::setfill('0') << y << "/";
        path << "z" << std::setw(4) << std::setfill('0') << z << "/";
        path << experimentName_
            << "_x" << std::setw(4) << std::setfill('0') << x
            << "_y" << std::setw(4) << std::setfill('0') << y
            << "_z" << std::setw(4) << std::setfill('0') << z
            << ".raw";
        return path.str();
    }
    
    std::string directory_;
    Roi<N> roi_;
    
    std::string experimentName_;
    double scale_[3];
    int boundary_[3];
    int magnification_;
    Roi<N> dataRoi_;
};

} /* namespace BW */

#endif /* BW_SOURCEKNOSSOS_H */
