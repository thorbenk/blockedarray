#ifndef BLOCKWISECC_H
#define BLOCKWISECC_H

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cassert>

#include <boost/foreach.hpp>

#include <vigra/union_find.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/labelvolume.hxx>
#include <vigra/labelimage.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/multi_pointoperators.hxx>

#include "compressedarray.h"

using vigra::detail::UnionFindArray;

/**
 * Region of interest
 * 
 * A ROI is defined by a (N-dimensional) rectangular region.
 * The lower left corner is Roi::p, the upper right cornver Roi::q
 * (with Roi::q being exclusive).
 */
template<int N>
class Roi {
    public:
    typedef vigra::TinyVector<int, N> V;
   
    /**
     * Region of interest [start, end)
     */
    Roi(V start, V end) : p(start), q(end) {}
   
    /**
     * Constructs an empty region of interest
     */
    Roi() {} 
   
    /**
     * equality
     */
    bool operator==(const Roi<N>& other) {
        return other.p == p && other.q == q;
    }
    
    /**
     * intersection of this Roi with 'other'
     * 
     * if an intersection exists, it is written into 'out'
     * 
     * returns: whether this roi and 'other' intersect.
     */
    bool intersect(const Roi& other, Roi& out) {
        for(int i=0; i<N; ++i) {
            out.p[i] = std::max(p[i], other.p[i]);
            out.q[i] = std::min(q[i], other.q[i]);
            if(out.p[i] >= q[i] || out.p[i] >= other.q[i] || out.q[i] < p[i] || out.q[i] < other.p[i]) {
                return false;
            }
        }
        return true;
    }
   
    /**
     * extent of this region of interest 
     */
    V shape() const {
        return q-p;
    }
    
    /**
     * remove the 'axis'-th dimension (counting from 0) 
     */
    Roi<N-1> removeAxis(int axis) const {
        Roi<N-1> ret;
        int j=0;
        for(int i=0; i<N; ++i) {
            if(i == axis) continue;
            ret.p[j] = p[i];
            ret.q[j] = q[i];
            ++j;
        }
        return ret;
    }
   
    /**
     * append a dimension/axis at the end, for which the region of interest
     * lies in [from, to) 
     */
    Roi<N+1> appendAxis(int from, int to) const {
        Roi<N+1> ret;
        std::copy(p.begin(), p.end(), ret.p.begin());
        std::copy(q.begin(), q.end(), ret.q.begin());
        ret.p[N] = from;
        ret.q[N] = to;
        return ret; 
    }
     
    /**
     * prepend a dimension/axis at the beginning, for which the region of interest
     * lies in [from, to) 
     */
    Roi<N+1> prependAxis(int from, int to) const {
        Roi<N+1> ret;
        std::copy(p.begin(), p.end(), ret.p.begin()+1);
        std::copy(q.begin(), q.end(), ret.q.begin()+1);
        ret.p[0] = from;
        ret.q[0] = to;
        return ret; 
    }
    
    V p;
    V q;
};

template<int N>
std::ostream& operator<<(std::ostream& o, const Roi<N>& roi) {
    o << roi.p << " -- " << roi.q;
    return o; 
}

/**
 * Computes a tiling of (possibly overlapping) blocks.
 */
template<int N>
class Blocking {
    public:
    typedef typename Roi<N>::V V;
    
    typedef std::pair<V, Roi<N> > Pair;
        
    Blocking() {}
    
    Blocking(Roi<N> roi, V blockShape, V overlap = V() )
        : roi_(roi)
        , blockShape_(blockShape)
        , overlap_(overlap)
    {
        
        V blockP = blockGivenCoordinateP(roi.p);
        V blockQ = blockGivenCoordinateQ(roi.q);
    
        V x = roi.p;
        int dim=N-1;
        addBlock(x);
        while(dim >= 0) {
            while(dim >= 0) {
                if(x[dim] >= blockQ[dim]-1) {
                    x[dim] = blockP[dim];
                    --dim;
                    continue;
                } else {
                    ++x[dim];
                    addBlock(x);
                    break;
                }
            }
            if(dim >= 0) {
                dim = N-1;
            }
        }
    }
    
    size_t numBlocks() const {
        return blocks_.size();
    }
    
    void pprint() {
        std::pair<V, Roi<N> > roi;
        BOOST_FOREACH(roi, blocks_) {
            std::cout << roi.first << ": " << roi.second << std::endl;
        }
    }
    
    std::vector< std::pair<V, Roi<N> > > blocks() const { return blocks_; }
    
    private:
        
    V blockGivenCoordinateP(V p) const {
        V c;
        for(int i=0; i<N; ++i) { c[i] = p[i]/blockShape_[i]; }
        return c;
    }

    V blockGivenCoordinateQ(V q) const {
        V c;
        for(int i=0; i<N; ++i) { c[i] = (q[i]-1)/blockShape_[i] + 1; }
        return c;
    }
    
    void addBlock(V x) {
        Roi<N> r;
        for(int i=0; i<N; ++i) {
            r.p[i] = x[i]*blockShape_[i];
            r.q[i] = std::min( (x[i]+1)*blockShape_[i]+overlap_[i], roi_.q[i] );
        }
        blocks_.push_back(std::make_pair(x, r) );
    }

    Roi<N> roi_;
    V blockShape_;
    V overlap_;
    
    std::vector< std::pair<V, Roi<N> > > blocks_;
};

template<int N, class T>
void blockMerge(
    UnionFindArray<int>& ufd,
    Roi<N> roi1,
    T offset1,
    const vigra::MultiArrayView<N,T>& block1, 
    Roi<N> roi2,
    T offset2,
    const vigra::MultiArrayView<N,T>& block2
) {
    typedef vigra::MultiArrayView<N,T> ArrayView;
   
    Roi<N> intersection;
    if(!roi1.intersect(roi2, intersection)) { return; }
    
    ArrayView ov1 = block1.subarray(intersection.p - roi1.p, intersection.q -roi1.p); 
    ArrayView ov2 = block2.subarray(intersection.p - roi2.p, intersection.q -roi2.p); 
    if(ov1.shape() != ov2.shape()) { throw std::runtime_error("err"); }
    
    typename ArrayView::iterator it1 = ov1.begin();
    typename ArrayView::iterator it2 = ov2.begin();
    for(; it1 != ov1.end(); ++it1, ++it2) {
        int x = *it1+offset1;
        int y = *it2+offset2;
        //if(*it1 == 0) ufd.makeUnion(0,y);
        //if(*it2 == 0) ufd.makeUnion(x,0);
        ufd.makeUnion(x, y);
    }
}

/**
 * Blockwise thresholding (not limited by RAM)
 * 
 * Reads data with dimension N, pixel type T in a block-wise fashion from a HDF5File,
 * performs thresholding, and writes to a chunked and compressed output file.
 */
template<int N, class T>
class BlockwiseThresholding {
    public:
    
    typedef typename Roi<N>::V V;
    BlockwiseThresholding(const std::string& hdf5file, const std::string& hdf5group, V blockShape)
        : hdf5file_(hdf5file)
        , hdf5group_(hdf5group)
        , blockShape_(blockShape)
    {
        using vigra::HDF5File;
        
        HDF5File f(hdf5file, HDF5File::OpenReadOnly);
        vigra::ArrayVector<hsize_t> sh = f.getDatasetShape(hdf5group);
        f.close();
        
        vigra_precondition(sh.size() == N, "dataset shape is wrong");
       
        std::copy(sh.begin(), sh.end(), shape_.begin());
        Roi<N> roi(V(), shape_);
        
        Blocking<N> bb(roi, blockShape, V());
        std::cout << bb.numBlocks() << std::endl;
        
        blocking_ = bb;
    }
    
    /**
     * Applies thresholding 'threshold'  to the image. If a pixel value is greater than 'threshold',
     * it is assigned the 'ifLower' value, otherwise the 'ifHigher' value. 
     */
    void run(T threshold, vigra::UInt8 ifLower, vigra::UInt8 ifHigher, const std::string& hdf5file, const std::string& hdf5group, int compression = 1) {
        using namespace vigra;
       
        HDF5File out(hdf5file, HDF5File::Open);
        std::cout << hdf5file << ", " << hdf5group << ", " << shape_ << blockShape_ << std::endl;
        out.createDataset<N, UInt8>(hdf5group, shape_, 0, blockShape_, compression);
        
        std::pair<V, Roi<N> > p;
        int blockNum = 0;
        BOOST_FOREACH(p, blocking_.blocks()) {
            std::cout << "  block " << blockNum+1 << "/" << blocking_.numBlocks() << "        \r" << std::flush;
            Roi<N> roi = p.second;
            MultiArray<N, T> inBlock(roi.q - roi.p);
            HDF5File in(hdf5file_, HDF5File::OpenReadOnly);
            in.readBlock(hdf5group_, roi.p, roi.q-roi.p, inBlock);
            in.close();
            
            MultiArray<N, UInt8> outBlock(inBlock.shape());
            T* a     = inBlock.data();
            UInt8* b = outBlock.data();
            for(size_t k=0; k<inBlock.size(); ++k) {
                *b = (*a > threshold) ? ifHigher : ifLower;
                ++a; ++b;
            }
            out.writeBlock(hdf5group, roi.p, outBlock);
            ++blockNum;
        }
        
        out.close();
    }
    
    private:
    std::string hdf5file_;
    std::string hdf5group_;
    V shape_;
    V blockShape_;
    Blocking<N> blocking_;
};

/**
 * Blockwise channel selector.
 * 
 * FIXME: Assumes channel is first axis
 */
template<int N, class T>
class BlockwiseChannelSelector {
    public:
    
    typedef typename Roi<N-1>::V V;
    
    BlockwiseChannelSelector(const std::string& hdf5file, const std::string& hdf5group, V blockShape)
        : hdf5file_(hdf5file)
        , hdf5group_(hdf5group)
        , blockShape_(blockShape)
    {
        using vigra::HDF5File;
        HDF5File f(hdf5file, HDF5File::OpenReadOnly);
        vigra::ArrayVector<hsize_t> sh = f.getDatasetShape(hdf5group);
        std::cout << "* blockwise channel selector on dataset with shape = " << sh << std::endl;
        f.close();
        vigra_precondition(sh.size() == N, "dataset shape is wrong");
        std::copy(sh.begin()+1, sh.end(), shape_.begin());
        std::cout << "  shape without channel: " << shape_ << std::endl;
        Roi<N-1> roi(V(), shape_);
        Blocking<N-1> bb(roi, blockShape, V());
        std::cout << bb.numBlocks() << std::endl;
        blocking_ = bb;
    }
    
    void run(int channel, const std::string& hdf5file, const std::string& hdf5group, int compression = 1) {
        using namespace vigra;
        using std::vector;
        
        vigra_precondition(compression >= 1 && compression <= 9, "compression must be >= 1 and <= 9");
        
        HDF5File out(hdf5file, HDF5File::Open);
        std::cout << hdf5file << ", " << hdf5group << ", " << shape_ << blockShape_ << std::endl;
        
        typename vigra::MultiArrayShape<N-1>::type sh, bShape;
        
        std::cout << hdf5group << ", " << shape_ << ", " << blockShape_ << std::endl;
        out.createDataset<N-1, T>(hdf5group, shape_, 0, blockShape_, compression);
        int blockNum = 0;
        
        vector<typename Blocking<N-1>::Pair> blocks = blocking_.blocks();
        
        for(int i=0; i<blocks.size(); ++i) {
            const Roi<N-1>& roi = blocks[i].second;
            
            std::cout << "  block " << i+1 << "/" << blocks.size() << "        \r" << std::flush;
           
            Roi<N> newRoi = roi.prependAxis(channel, channel+1);
            
            MultiArray<N, T> inBlock(newRoi.q - newRoi.p);
            HDF5File in(hdf5file_, HDF5File::OpenReadOnly);
            in.readBlock(hdf5group_, newRoi.p, newRoi.q-newRoi.p, inBlock);
            in.close();
            
            out.writeBlock(hdf5group, roi.p, inBlock.template bind<0>(0));
            ++blockNum;
        }
        out.close();
    }
    
    private:
    std::string hdf5file_;
    std::string hdf5group_;
    V shape_;
    V blockShape_;
    Blocking<N-1> blocking_;
};

/**
 * Interface to obtain a block of data given a region of interest
 */
template<int N, class T>
class BlockedSource {
    public:
    typedef typename Roi<N>::V V;
        
    BlockedSource() {}
    virtual ~BlockedSource() {};
    
    virtual V shape() const { return V(); };
    virtual bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const { return true; };
};


/**
 * Interface to write a block of data given a region of interest
 */
template<int N, class T>
class BlockedSink {
    public:
    typedef typename Roi<N>::V V;
        
    BlockedSink() {}
    virtual ~BlockedSink() {};
   
    void setShape(V shape) {
        shape_ = shape; 
    }
    
    void setBlockShape(V shape) {
        blockShape_ = shape;
    }
    
    V shape() const { return V(); };
    V blockShape() const { return V(); };
    
    virtual bool writeBlock(Roi<N> roi, const vigra::MultiArrayView<N,T>& block) const { return true; };
    
    protected:
    V shape_;
    V blockShape_;
};

/**
 * Read a block of data from a HDF5File
 */
template<int N, class T>
class HDF5BlockedSource : public BlockedSource<N,T> {
    public:
    typedef typename BlockedSource<N,T>::V V;
    
    HDF5BlockedSource(const std::string& hdf5file, const std::string& hdf5group)
        : BlockedSource<N,T>()
        , hdf5file_(hdf5file)
        , hdf5group_(hdf5group)
    {
    }
    
    virtual V shape() const {
        using namespace vigra;
        
        V ret;
        HDF5File f(hdf5file_, HDF5File::OpenReadOnly);
        vigra::ArrayVector<hsize_t> sh = f.getDatasetShape(hdf5group_);
        f.close();
        vigra_precondition(sh.size() == N, "dataset shape is wrong");
        std::copy(sh.begin(), sh.end(), ret.begin());
        return ret;
    }
    
    virtual bool readBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const {
        using namespace vigra;
        
        HDF5File in(hdf5file_, HDF5File::OpenReadOnly);
        in.readBlock(hdf5group_, roi.p, roi.q-roi.p, block);
        in.close();
        return true;
    }
    
    private:
    std::string hdf5file_;
    std::string hdf5group_;
};

/**
 * Write a block of data to a HDF5File
 */
template<int N, class T>
class HDF5BlockedSink : public BlockedSink<N,T> {
    public:
    typedef typename BlockedSink<N,T>::V V;

    HDF5BlockedSink(const std::string& hdf5file, const std::string& hdf5group, int compression = 1)
        : BlockedSink<N,T>()
        , hdf5file_(hdf5file)
        , hdf5group_(hdf5group)
        , compression_(compression)
        , fileCreated_(false)
    {
        vigra_precondition(compression >= 1 && compression <= 9, "compression must be >= 1 and <= 9");
    }

    virtual bool writeBlock(Roi<N> roi, vigra::MultiArrayView<N,T>& block) const {
        using namespace vigra;
        if(!fileCreated_) {
            std::cout << "* write " << hdf5file_ << "/" << hdf5group_ << std::endl;
            HDF5File out(hdf5file_, HDF5File::Open);
            out.createDataset<N, T>(hdf5group_, this->shape(), 0, this->blockShape(), compression_);
            out.close();
        }
        
        HDF5File out(hdf5file_, HDF5File::Open);
        out.writeBlock(hdf5group_, roi.p, block);
        out.close();
    }
        
    private:
    std::string hdf5file_;
    std::string hdf5group_;
    int compression_;
    V shape_;
    bool fileCreated_;
};

/**
 * Compute connected components either in 2D or 3D using 4 or 6 neighborhood.
 */
template<int N, class T, class LabelType>
struct ConnectedComponents {
    static LabelType compute(const vigra::MultiArrayView<N,T>& in, vigra::MultiArrayView<N,LabelType>& out);
};

template<class T, class LabelType>
struct ConnectedComponents<2,T,LabelType> {
    static LabelType compute(const vigra::MultiArrayView<2,T>& in, vigra::MultiArrayView<2,LabelType>& out) {
        return vigra::labelImageWithBackground( in, out, false /*use 4-neighbors*/, 0 /*bg label*/);
    }
};

template<class T, class LabelType>
struct ConnectedComponents<3,T,LabelType> {
    static LabelType compute(const vigra::MultiArrayView<3,T>& in, vigra::MultiArrayView<3,LabelType>& out) {
         return vigra::labelVolumeWithBackground(in, out, vigra::NeighborCode3DSix(), 0);
    }
};

/**
 * Compute connected components block-wise (less limited to RAM)
 */
template<int N, class T>
class BlockwiseConnectedComponents {
    public:
    typedef typename Roi<N>::V V;
    
    typedef int LabelType;
    
    typedef CompressedArray<N,LabelType> Compressed;
        
    BlockwiseConnectedComponents(BlockedSource<N,T>* blockProvider, V blockShape)
        : blockProvider_(blockProvider)
        , blockShape_(blockShape)
    {
        using namespace vigra;
        using std::vector;
        using std::pair;
       
        //blocks are overlapping by 1 pixel in all directions 
        V overlap; std::fill(overlap.begin(), overlap.end(), 1);
       
        //
        // run connected components on each block separately
        //
        
        Blocking<N> blocking( Roi<N>(V(), blockProvider->shape()), blockShape, overlap);
        blockRois_ = blocking.blocks();
        vector<LabelType> offsets(blockRois_.size()+1);
        ccBlocks_.resize(blockRois_.size());
        
        std::cout << "* run connected components on each of " << blockRois_.size() << " blocks separately" << std::endl;
        
        size_t sizeBytes = 0;
        size_t sizeBytesUncompressed = 0;
        
        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << " (#components: " << offsets[i] << ", curr compressed size: " << sizeBytes/(1024.0*1024.0) << " MB)                  \r" << std::flush;
            Roi<N>& block = blockRois_[i].second;
            //std::cout << "  - CC on block " << block << std::endl;
            
            //int maxLabel = vigra::labelVolumeWithBackground(
            //    vigra::srcMultiArrayRange( labels_.subarray(block.p, block.q) ),
            //    vigra::destMultiArray( ccBlocks[i] ), vigra::NeighborCode3DSix(), 0);
        
            MultiArray<N, T> inBlock(block.q-block.p);
            blockProvider_->readBlock(block, inBlock);
            
            MultiArray<N, LabelType> cc(block.q-block.p); 
           
            LabelType maxLabel = ConnectedComponents<N, T, LabelType>::compute(inBlock, cc);
            //LabelType maxLabel = vigra::labelImageWithBackground(
            //    inBlock,
            //    cc,
            //    false /*use 4-neighbors*/,
            //    0 /*bg label*/);
        
            //ccBlocks_[i] = cc;
            
            ccBlocks_[i] = Compressed(cc);
            ccBlocks_[i].compress();
            
            sizeBytes += ccBlocks_[i].currentSizeBytes();
            sizeBytesUncompressed += ccBlocks_[i].uncompressedSizeBytes();
            
            offsets[i+1] = offsets[i] + maxLabel + 1;
        }
        std::cout << std::endl;
        std::cout << "  " << sizeBytes/(1024.0*1024.0) << " MB (vs. " << sizeBytesUncompressed/(1024.0*1024.0) << "MB uncompressed)" << std::endl;
        
        LabelType maxOffset = offsets[offsets.size()-1];
        
        std::cout << "  initialize union find datastructure with maxOffset = " << maxOffset << std::endl;
        UnionFindArray<LabelType> ufd(maxOffset);
        
        //
        // merge adjacent blocks 
        //
        std::cout << "* merge adjacent blocks" << std::endl;
        
        //FIXME: use adjacency relation of blocks
        std::vector< std::pair<size_t, size_t> > adjBlocks;
        for(size_t i=0; i<blockRois_.size(); ++i) {
            for(int j=i+1; j<blockRois_.size(); ++j) {
                V bc1 = blockRois_[i].first;
                V bc2 = blockRois_[j].first;
                
                if(std::abs(vigra::squaredNorm(bc1-bc2)) == 1) {
                    adjBlocks.push_back(std::make_pair(i,j));
                }
            }
        }
               
        for(vector<pair<size_t, size_t> >::iterator it=adjBlocks.begin(); it!=adjBlocks.end(); ++it) {
            std::cout << "  pair " << std::distance(adjBlocks.begin(), it)+1 << "/" << adjBlocks.size() << "        \r" << std::flush;
            size_t i = it->first;
            size_t j = it->second;
        
            Roi<N>& block1 = blockRois_[i].second;
            Roi<N>& block2 = blockRois_[j].second;
            
            //cc1 = ccBlocks_[i];
            //cc2 = ccBlocks_[j];
            MultiArray<N,LabelType> cc1(block1.q-block1.p);
            MultiArray<N,LabelType> cc2(block2.q-block2.p);
            ccBlocks_[i].readArray(cc1);
            ccBlocks_[j].readArray(cc2);
            
            blockMerge<N, LabelType>(ufd,
                block1, offsets[i], cc1,
                block2, offsets[j], cc2);
        }
        std::cout << std::endl;
        
        LabelType maxLabel = ufd.makeContiguous();
       
        std::cout << "* relabel" << std::endl;
        sizeBytes = 0;
        sizeBytesUncompressed = 0;
        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << "        \r" << std::flush;
            Roi<N>& roi = blockRois_[i].second;
            
            //MultiArray<N,LabelType> cc = ccBlocks_[i];
            MultiArray<N,LabelType> cc(roi.q-roi.p); ccBlocks_[i].readArray(cc);
            
            LabelType offset = offsets[i];
        
            for(size_t j=0; j<cc.size(); ++j) { cc[j] = ufd[ cc[j]+offset ]; }
            
            //ccBlocks_[i] = cc;
            ccBlocks_[i] = Compressed(cc);
            ccBlocks_[i].compress();
            sizeBytes += ccBlocks_[i].currentSizeBytes();
            sizeBytesUncompressed += ccBlocks_[i].uncompressedSizeBytes();
        }
        std::cout << std::endl;
        std::cout << "  " << sizeBytes/(1024.0*1024.0) << " MB (vs. " << sizeBytesUncompressed/(1024.0*1024.0) << "MB uncompressed)" << std::endl;
        
    }
    
    void writeResult(const std::string& hdf5file, const std::string& hdf5group, int compression = 1) {
        using namespace vigra;
        
        vigra_precondition(compression >= 1 && compression <= 9, "compression must be >= 1 and <= 9");
        
        std::cout << "* write " << hdf5file << "/" << hdf5group << std::endl;
        HDF5File out(hdf5file, HDF5File::Open);
        out.createDataset<N, LabelType>(hdf5group, blockProvider_->shape(), 0, blockShape_, compression);
        for(size_t i=0; i<blockRois_.size(); ++i) {
            std::cout << "  block " << i+1 << "/" << blockRois_.size() << "        \r" << std::flush;
            const Roi<N>& roi = blockRois_[i].second;
            
            //cc = ccBlocks_[i];
            MultiArray<N,LabelType> cc(roi.q-roi.p);
            ccBlocks_[i].readArray(cc);
            
            out.writeBlock(hdf5group, roi.p, cc);
        
        }
        out.close();
        std::cout << std::endl;
    }
    
    private:
    BlockedSource<N,T>* blockProvider_;
    V blockShape_;
    
    std::vector< std::pair<V, Roi<N> > > blockRois_;
    //std::vector< vigra::MultiArray<N,LabelType> > ccBlocks_;
    std::vector< Compressed > ccBlocks_;
};

#endif /* BLOCKWISECC_H */
