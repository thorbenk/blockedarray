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

#ifndef BW_REGIONFEATURES2_H
#define BW_REGIONFEATURES2_H

#include <vigra/accumulator.hxx>
#include <vigra/multi_array.hxx>

#include <bw/source.h>
#include <bw/sink.h>
#include <bw/blocking.h>

namespace BW {

using namespace vigra::acc;

static const int StaticHistogramSize = 0;
static const int DynamicHistogramSize = 50;

template<int N, class T, class U>
class RegionFeatures {
    public:

    typedef Select<Count,
                   Mean,
                   Variance,
                   Skewness,
                   Kurtosis,
                   Minimum,
                   Maximum,
                   UserRangeHistogram<StaticHistogramSize>,
                   StandardQuantiles<UserRangeHistogram<StaticHistogramSize> >, //vector of length 7
                   RegionCenter, //center of region (pixel coordinates)
                   RegionRadii, //3 numbers per region
                   RegionAxes, //3 x 3 numbers per region
                   Weighted<RegionCenter>, //weighted center of region
                   Weighted<RegionRadii>,
                   Weighted<RegionAxes>,
                   Select<Coord<Minimum>, //bounding box min
                          Coord<Maximum>, //bounding box max
                          Coord<ArgMinWeight>, //pixel where the data has minimum value within region
                          Coord<ArgMaxWeight> //pixel where the data has maximum value within region
                   >,
                   DataArg<1>, WeightArg<1>, LabelArg<2>
                   > ScalarRegionAccumulatorsBW;

    typedef vigra::acc::AccumulatorChainArray<
                vigra::CoupledArrays<3, T, U>,
                ScalarRegionAccumulatorsBW
            > AccChain;

    typedef typename Roi<N>::V V;
    RegionFeatures(Source<N,T>* dataSource, Source<N,U>* labelsBlockSource, V blockShape)
        : blockShape_(blockShape)
        , dataSource_(dataSource)
        , labelsBlockSource_(labelsBlockSource)
    {
        vigra_precondition(dataSource_->shape() == labelsBlockSource_->shape(), "shapes do not match");

        Roi<N> roi(V(), dataSource_->shape());
        Blocking<N> bb(roi, blockShape, V());
        std::cout << "* RegionFeatures with " << bb.numBlocks() << " blocks" << std::endl;
        blocking_ = bb;
    }

    void run(const std::string& filename) {
        using namespace vigra;

        accumulators_.resize(blocking_.numBlocks());

        typename Blocking<N>::Pair p;

        T m = 0;
        T M = vigra::NumericTraits<T>::min();
        size_t i=0;

        //MultiArray<N, T> dataBlock(blockShape_);

        BOOST_FOREACH(p, blocking_.blocks()) {
            std::cout << "  cc block " << i+1 << "/" << blocking_.numBlocks() << "        \r" << std::flush;
            Roi<N> roi = p.second;
            //if(dataBlock.shape() !=roi.shape()) {
            //	dataBlock.reshape(roi.shape());
            //}
            MultiArray<N, T> dataBlock(roi.shape());
            dataSource_->readBlock(roi, dataBlock);
            M = std::max(M, *std::max_element(dataBlock.begin(), dataBlock.end()));
            ++i;
        }
	    std::cout << std::endl;

        vigra::HistogramOptions histogram_opt;
        histogram_opt = histogram_opt.setBinCount(DynamicHistogramSize);
        histogram_opt = histogram_opt.setMinMax(m,M);

        i = 0;
        BOOST_FOREACH(p, blocking_.blocks()) {
            std::cout << "  block " << i+1 << "/" << blocking_.numBlocks() << "        \r" << std::flush;

            Roi<N> roi = p.second;

            MultiArray<N, T> dataBlock(roi.shape());
            dataSource_->readBlock(roi, dataBlock);

            MultiArray<N, U> labelsBlock(roi.shape());
            labelsBlockSource_->readBlock(roi, labelsBlock);

            accumulators_[i].ignoreLabel(0);
            accumulators_[i].setHistogramOptions(histogram_opt);
            accumulators_[i].setCoordinateOffset(roi.p);

            extractFeatures(dataBlock, labelsBlock, accumulators_[i]);

            ++i;
        }
        std::cout << std::endl;

        vigra::MultiArrayIndex maxRegionLabel = 0;
        for(i=0; i<accumulators_.size(); ++i) {
            maxRegionLabel = std::max(maxRegionLabel, accumulators_[i].maxRegionLabel());
        }

        AccChain a;
        a.setMaxRegionLabel(maxRegionLabel);
        a.setHistogramOptions(histogram_opt);
        a.ignoreLabel(0);

        for(i=0; i<accumulators_.size(); ++i) {
            std::cout << "  acc " << i+1 << "/" << blocking_.numBlocks() << " has " << accumulators_[i].regionCount() << " regions          \r" << std::flush;

            std::vector<size_t> relabeling(accumulators_[i].maxRegionLabel()+1);
            vigra::linearSequence(relabeling.begin(), relabeling.end());
            a.merge(accumulators_[i], relabeling);

        }
        std::cout << std::endl;

        std::cout << "write features to " << filename << std::endl;
        vigra::HDF5File f(filename, vigra::HDF5File::Open);
        {
            vigra::MultiArray<1, float> count(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { count(i) = vigra::acc::get<vigra::acc::Count>(a, i); }
            f.write("count", count);
        }
        {
            vigra::MultiArray<1, float> mean(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { mean(i) = vigra::acc::get<vigra::acc::Mean>(a, i); }
            f.write("mean", mean);
        }
        {
            vigra::MultiArray<1, float> variance(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { variance(i) = vigra::acc::get<vigra::acc::Variance>(a, i); }
            f.write("variance", variance);
        }
        {
            vigra::MultiArray<1, float> skewness(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { skewness(i) = vigra::acc::get<vigra::acc::Skewness>(a, i); }
            f.write("skewness", skewness);
        }
        {
            vigra::MultiArray<1, float> kurtosis(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { kurtosis(i) = vigra::acc::get<vigra::acc::Kurtosis>(a, i); }
            f.write("kurtosis", kurtosis);
        }
        {
            vigra::MultiArray<1, float> minimum(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { minimum(i) = vigra::acc::get<vigra::acc::Minimum>(a, i); }
            f.write("minimum", minimum);
        }
        {
            vigra::MultiArray<1, float> maximum(vigra::Shape1(a.regionCount()));
            for(size_t i=0; i<a.regionCount(); ++i) { maximum(i) = vigra::acc::get<vigra::acc::Maximum>(a, i); }
            f.write("maximum", maximum);
        }
        {
            vigra::MultiArray<2, float> hist(vigra::Shape2(a.regionCount(), DynamicHistogramSize));
            for(size_t i=0; i<a.regionCount(); ++i) {
                for(size_t j=0; j<DynamicHistogramSize; ++j) {
                    hist(i, j) = vigra::acc::get<vigra::acc::UserRangeHistogram<StaticHistogramSize> >(a, i)[j];
                }
            }
            f.write("histogram", hist);
        }
        {
            vigra::MultiArray<2, float> quantiles(vigra::Shape2(a.regionCount(), 7));
            for(size_t i=0; i<a.regionCount(); ++i) {
                for(size_t j=0; j<7; ++j) {
                    quantiles(i, j) = vigra::acc::get<StandardQuantiles<vigra::acc::UserRangeHistogram<StaticHistogramSize> > >(a, i)[j];
                }
            }
            f.write("quantiles", quantiles);
        }
        {
            vigra::MultiArray<2, float> regionCenter(vigra::Shape2(a.regionCount(), 3));
            for(size_t i=0; i<a.regionCount(); ++i) {
                for(size_t j=0; j<3; ++j) {
                    regionCenter(i, j) = vigra::acc::get<RegionCenter>(a, i)[j];
                }
            }
            f.write("regionCenter", regionCenter);
        }

        {
            vigra::MultiArray<2, float> regionRadii(vigra::Shape2(a.regionCount(), 3));
            for(size_t i=0; i<a.regionCount(); ++i) {
                for(size_t j=0; j<3; ++j) {
                    regionRadii(i, j) = vigra::acc::get<RegionRadii>(a, i)[j];
                }
            }
            f.write("regionRadii", regionRadii);
        }

        {
            vigra::MultiArray<3, float> regionAxes(vigra::Shape3(a.regionCount(), 3, 3));
            for(size_t i=0; i<a.regionCount(); ++i) {
                for(size_t j=0; j<3; ++j) {
                    for(size_t k=0; k<3; ++k) {
                        regionAxes(i, j, k) = vigra::acc::get<RegionAxes>(a, i)(j,k);
                    }
                }
            }
            f.write("regionAxes", regionAxes);
        }
        f.close();

        std::cout << "....." << std::endl;

    }

    private:
    V shape_;
    V blockShape_;
    Blocking<N> blocking_;

    Source<N,T>* dataSource_;
    Source<N,U>* labelsBlockSource_;

    std::vector<AccChain> accumulators_;
};

} /* namespace BW */

#endif /* BW_REGIONFEATURES2_H */
