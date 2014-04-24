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

#ifndef BW_MESHEXTRACTOR_H
#define BW_MESHEXTRACTOR_H

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>

#include <vigra/hdf5impex.hxx>

#include <bw/source.h>
#include <bw/blocking.h>

typedef vigra::TinyVector<vigra::MultiArrayIndex, 3> Coor;

namespace boost {

template<>
struct hash<Coor> {
    inline std::size_t operator()(const Coor& k) const {
        size_t h = 0;
        hash_combine(h, k[0]);
        hash_combine(h, k[1]);
        hash_combine(h, k[2]);
        return h;
    }
};

template<>
struct hash<std::pair<size_t, size_t> > {
    inline std::size_t operator()(const std::pair<size_t, size_t>& k) const {
        size_t h = 0;
        hash_combine(h, k.first);
        hash_combine(h, k.second);
        return h;
    }
};

} /* namespace boost */


namespace detail {

typedef boost::unordered_map<Coor, size_t> CoorMap;
typedef boost::unordered_set<std::pair<size_t, size_t> > LinesSet;

short normalOrientation(Coor tc);
void cartesianCorners(Coor tc, std::vector<Coor>& corners);

template<class T>
void
extractMesh(
    const vigra::MultiArrayView<3, T>& labels, 
    Coor offset,
    T label,
    CoorMap& h,
    LinesSet& lines,
    std::vector<std::vector<size_t> >& faces
) {
    using namespace vigra;
    
    size_t n = 0;
    std::vector<Coor> cornerPoints(4);
    std::vector<size_t> cornerIds(4);
    std::vector<size_t> face(4);
    
    const Coor offsets[3] = { Coor(1,0,0), Coor(0,1,0), Coor(0,0,1) };
   
    //coordinate of current voxel
    Coor c(0,0,0);
    //coordinate of adjacent voxel
    Coor d;
    for(MultiArrayIndex i=0; i<labels.shape(0)-1; ++i) {
        c[0] = i;
        for(MultiArrayIndex j=0; j<labels.shape(1)-1; ++j) {
            c[1] = j;
            for(MultiArrayIndex k=0; k<labels.shape(2)-1; ++k) {
                c[2] = k;
                
                for(MultiArrayIndex l=0; l<3; ++l) {
                    d = c + offsets[l];
                    
                    if( ! ((labels[c] == label && labels[d] != label) || (labels[d] == label && labels[c] != label) ) ) { continue; }
                
                    Coor tc = (2*(c+offset) + 2*(d+offset))/2;
                    cartesianCorners(tc, cornerPoints);
                    
                    //assign unique, consecutive vertex IDs 
                    //to the corner points
                    for(size_t m=0; m<4; ++m) {
                        const Coor& coor = cornerPoints[m];
                        const CoorMap::const_iterator it = h.find(coor);
                        if(it == h.end()) {
                            h[coor] = h.size()-1;
                        }
                    }
                    //build line and face structures by referencing
                    //the vertex IDs
                    for(size_t m=0; m<4; ++m) {
                        size_t n = (m+1) % 4;
                        const Coor& coor0 = cornerPoints[m];
                        const Coor& coor1 = cornerPoints[n];
                        
                        size_t id0 = h.at(coor0);
                        size_t id1 = h.at(coor1);
                        
                        face[m] = id0;
                        
                        if(id0 > id1) { std::swap(id0, id1); }
                        lines.insert( std::make_pair(id0, id1) );
                    }
                    faces.push_back(face);
                }
            }
        }
    }
}

} /* namespace detail */

namespace BW {

/**
 *  extract the mesh of an object's boundary (not limited by RAM)
 */
template<int N, class T>
class MeshExtractor {
    public:

    typedef typename Roi<N>::V V;
    MeshExtractor(Source<N,T>* source, V blockShape)
        : blockShape_(blockShape)
        , shape_(source->shape())
        , source_(source)
    {
        vigra_precondition(shape_.size() == N, "dataset shape is wrong");

        Roi<N> roi(V(), shape_);
        Blocking<N> bb(roi, blockShape, V(1,1,1));
        std::cout << "* Thresholding with " << bb.numBlocks() << " blocks" << std::endl;
        blocking_ = bb;
    }

    void run(T object, const std::string& filename) {
        using detail::CoorMap;
        using detail::LinesSet;
        using detail::extractMesh;
        using namespace vigra;
        
        CoorMap h;
        LinesSet lines;
        std::vector<std::vector<size_t> > faces;

        int blockNum = 0;
        typename Blocking<N>::Pair p;
        BOOST_FOREACH(p, blocking_.blocks()) {
            std::cout << "  block " << blockNum+1 << "/" << blocking_.numBlocks()
                      << " | verts = " << h.size() 
                      << "  faces = " << faces.size()
                      << "            \r" << std::flush;
            Roi<N> roi = p.second;

            MultiArray<N, T> inBlock(roi.shape());
            source_->readBlock(roi, inBlock);
            
            extractMesh(inBlock, roi.p, object, h, lines, faces);

            ++blockNum;
        }
        
        std::cout << std::endl;
        std::cout << "writing mesh to file " << filename << std::endl;
        
        vigra::MultiArray<2, uint32_t> outVerts(vigra::Shape2(h.size(), 3));
        vigra::MultiArray<2, uint32_t> outFaces(vigra::Shape2(faces.size(), 4));
        vigra:MultiArray<2, uint32_t> outLines(vigra::Shape2(lines.size(), 2));
        {
            size_t i = 0;
            for(CoorMap::const_iterator it = h.begin(); it != h.end(); ++it, ++i) {
                for(size_t j=0; j<3; ++j) {
                    outVerts(it->second, j) = it->first[j];
                }
            }
        }
        {
            size_t i = 0;
            for(LinesSet::const_iterator l = lines.begin(); l != lines.end(); ++l) {
                outLines(i,0) = l->first;
                outLines(i,1) = l->second;
                ++i;
            }
        }
        {
            size_t i = 0;
            for(typename std::vector<std::vector<size_t> >::const_iterator face = faces.begin(); face != faces.end(); ++face) {
                for(size_t j=0; j<4; ++j) {
                    outFaces(i, j) = (*face)[j];
                }
                ++i;
            }
        }
        
        vigra::HDF5File f(filename, vigra::HDF5File::Open);
        f.write("faces", outFaces);
        f.write("lines", outLines);
        f.write("verts", outVerts);
        f.close();
    }

    private:
    V shape_;
    V blockShape_;
    Blocking<N> blocking_;
    Source<N,T>* source_;
};

} /* namespace BW */

#endif /* BW_MESHEXTRACTOR */
