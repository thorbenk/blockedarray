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

#define PY_ARRAY_UNIQUE_SYMBOL blockedarray_PyArray_API
#define NO_IMPORT_ARRAY

#include "Python.h"

#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>
#include <vigra/python_utility.hxx>

//#define DEBUG_PRINTS

#include <bw/array.h>

#include "dtypename.h"

#include <bw/extern_templates.h>

using namespace BW;

template<int N, typename T>
boost::python::tuple blockListToPython(
    const Array<N,T>& ba,
    const typename Array<N,T>::BlockList& bL) {
    vigra::NumpyArray<2, vigra::UInt32> start(vigra::Shape2(bL.size(), N));
    vigra::NumpyArray<2, vigra::UInt32> stop(vigra::Shape2(bL.size(), N));
    for(int i=0; i<bL.size(); ++i) {
        typename Array<N,T>::V pp, qq;
        ba.blockBounds(bL[i], pp, qq);
        for(int j=0; j<N; ++j) {
            start(i,j) = pp[j];
            stop(i,j) = qq[j];
        }
    }
    return boost::python::make_tuple(start, stop);
}

template<int N, typename T>
boost::python::list voxelValuesToPython(
    const typename Array<N,T>::VoxelValues& vv
) {
    typedef vigra::NumpyArray<1, uint32_t> PyCoord;
    typedef vigra::NumpyArray<1, T> PyVal;
    typedef vigra::Shape1 Sh;

    PyCoord coords[N];
    for(int i=0; i<N; ++i) {
        coords[i].reshape(Sh(vv.first.size()));
    }
    PyVal vals(Sh(vv.first.size()));
    for(size_t j=0; j<N; ++j) {
        PyCoord& c = coords[j];
        for(size_t i=0; i<vv.first.size(); ++i) {
            c[i] = vv.first[i][j];
        }
    }
    for(size_t i=0; i<vv.first.size(); ++i) {
        vals[i] = vv.second[i];
    }

    //convert to python list
    boost::python::list l;
    for(int i=0; i<N; ++i) {
        l.append(coords[i]);
    }
    l.append(vals);
    return l;
}

template<int N, class T>
struct PyBlockedArray {
    typedef Array<N, T> BA;
    typedef typename BA::V V;

    /**
     * Convert from a python sequence into the appropriate shape type.
     * This is useful when the python sequence contains numpy int types
     * (e.g. numpy.uint32), which are not automatically registered for
     * conversion in vigranumpy/src/core/converters.cxx.
     *
     * Ideally, such types would be registered via the usual boost::python
     * mechanism (for example, like in http://github.com/ndarray/Boost.NumPy).
     * For now, this function is the workaround suggested from
     * http://shitohichiumaya.blogspot.com/2012/01/passing-user-defined-python-object-to-c_4561.html
     */
    static V extractCoordinate(boost::python::object const & seq)
    {
        int length = boost::python::extract<int>( seq.attr("__len__")() );
        vigra_precondition(length==N, "coordinate sequence has wrong length");

        V coord;
        for(int i=0; i<N; ++i)
        {
        	const boost::python::object obj = seq.attr("__getitem__")(i);
            coord[i] = boost::python::extract<typename V::value_type>( obj.attr("__int__")() );
        }
        return coord;
    }

    static void readSubarray(BA& ba, boost::python::object p, boost::python::object q, vigra::NumpyArray<N, T> out)
    {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);
    	ba.readSubarray(_p, _q, out);
    }

    static void deleteSubarray(BA& ba, boost::python::object p, boost::python::object q)
    {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);
    	ba.deleteSubarray(_p, _q);
    }
        static void writeSubarray(BA& ba, boost::python::object p, boost::python::object q, vigra::NumpyArray<N, T> a
    ) {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);
        ba.writeSubarray(_p, _q, a);
    }

    static void writeSubarrayNonzero(BA& ba, boost::python::object p, boost::python::object q, vigra::NumpyArray<N, T> a, T writeAsZero
    ) {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);
        ba.writeSubarrayNonzero(_p, _q, a, writeAsZero);
    }

    static void sliceToPQ(boost::python::tuple sl, V &p, V &q) {
        vigra_precondition(boost::python::len(sl)==N, "tuple has wrong length");
        for(int k=0; k<N; ++k) {
            boost::python::slice s = boost::python::extract<boost::python::slice>(sl[k]);
            p[k] = boost::python::extract<int>(s.start());
            q[k] = boost::python::extract<int>(s.stop());
        }
    }

    static vigra::NumpyAnyArray getitem(BA& ba, boost::python::tuple sl) {
        V p,q;
        sliceToPQ(sl, p, q);
        vigra::NumpyArray<N,T> out(q-p);
        ba.readSubarray(p, q, out);
        return out;
    }

    static void setDirty(BA& ba, boost::python::tuple sl, bool dirty) {
        V p,q;
        sliceToPQ(sl, p, q);
        ba.setDirty(p,q,dirty);
    }

    static void setitem(BA& ba, boost::python::tuple sl, vigra::NumpyArray<N,T> a) {
        V p,q;
        sliceToPQ(sl, p, q);
        ba.writeSubarray(p, q, a);
    }

    static PyObject* blockShape(BA& ba) {
    	return shapeToPythonTuple( ba.blockShape() ).release();
    }

    static boost::python::tuple blocks(BA& ba, boost::python::object p, boost::python::object q) {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);
        return blockListToPython(ba, ba.blocks(_p, _q));
    }

    static boost::python::tuple dirtyBlocks(BA& ba, boost::python::object p, boost::python::object q) {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);
        return blockListToPython(ba, ba.dirtyBlocks(_p, _q));
    }

    static boost::python::tuple minMax(const BA& ba) {
        std::pair<T,T> mm = ba.minMax();
        return boost::python::make_tuple(mm.first, mm.second);
    }

    static boost::python::list nonzero(const BA& ba) {
        return voxelValuesToPython<N,T>(ba.nonzero());
    }

    static void applyRelabeling(BA& ba, vigra::NumpyArray<1, T> relabeling) {
        ba.applyRelabeling(relabeling);
    }

    static boost::python::list enumerateBlocksInRange(const BA& ba, boost::python::object p, boost::python::object q) {
    	V _p = extractCoordinate(p);
    	V _q = extractCoordinate(q);

    	std::vector<V> blocks = ba.enumerateBlocksInRange(_p, _q);
    	boost::python::list blockList ;
    	for (int i=0; i < blocks.size(); ++i)
    	{
    		boost::python::list coord;
    		for (int c=0; c < N; ++c)
    		{
    			coord.append(blocks[i][c]);
    		}
    		blockList.append(coord);
    	}
    	return blockList;
    }

    static boost::shared_ptr<BA> init( boost::python::object const & blockshape )
    {
    	return boost::shared_ptr<BA>( new BA( extractCoordinate(blockshape) ) );
    }
    
    static boost::shared_ptr<BA> initEmpty()
    {
        return boost::shared_ptr<BA>( new BA() );
    }
};

template<int N, class T>
void export_blockedArray() {
    typedef Array<N, T> BA;
    typedef PyBlockedArray<N,T> PyBA;

    using namespace boost::python;
    using namespace vigra;

    std::stringstream name; name << "BlockedArray" << N << DtypeName<T>::dtypeName();

    class_<BA, boost::shared_ptr<BA> >(name.str().c_str(), no_init) // No auto-provided init.  Use make_constructor, below.
    	.def("__init__", make_constructor(&PyBA::init))
        .def("__init__", make_constructor(&PyBA::initEmpty))
        .def("setDeleteEmptyBlocks", &BA::setDeleteEmptyBlocks,
             (arg("deleteEmpty")))
        .def("setCompressionEnabled", &BA::setCompressionEnabled,
             (arg("enableCompression")))
        .def("setMinMaxTrackingEnabled", &BA::setMinMaxTrackingEnabled,
             (arg("enableMinMaxTracking")))
        .def("setManageCoordinateLists", &BA::setManageCoordinateLists,
             (arg("manageCoordinateLists")))
        .def("minMax", &PyBA::minMax)
        .def("averageCompressionRatio", &BA::averageCompressionRatio)
        .def("numBlocks", &BA::numBlocks)
        .def("sizeBytes", &BA::sizeBytes)
        .def("blockShape", &PyBA::blockShape)
        .def("writeSubarray", registerConverters(&PyBA::writeSubarray))
        .def("writeSubarrayNonzero", registerConverters(&PyBA::writeSubarrayNonzero),
            (arg("p"), arg("q"), arg("a"), arg("writeAsZero")))
        .def("readSubarray", registerConverters(&PyBA::readSubarray))
        .def("deleteSubarray", registerConverters(&PyBA::deleteSubarray))
        .def("applyRelabeling", registerConverters(&PyBA::applyRelabeling),
            (arg("relabeling")))
        .def("__getitem__", registerConverters(&PyBA::getitem))
        .def("__setitem__", registerConverters(&PyBA::setitem))
        .def("setDirty", registerConverters(&PyBA::setDirty))
        .def("blocks", registerConverters(&PyBA::blocks))
        .def("dirtyBlocks", registerConverters(&PyBA::dirtyBlocks))
        .def("nonzero", registerConverters(&PyBA::nonzero))
        .def("enumerateBlocksInRange", registerConverters(&PyBA::enumerateBlocksInRange))
        .def("writeHDF5", &BA::writeHDF5)
        .def("readHDF5", &BA::writeHDF5)
        .staticmethod("readHDF5")
    ;
}

void export_blockedArray() {
    export_blockedArray<2, vigra::UInt8>();
    export_blockedArray<3, vigra::UInt8>();
    export_blockedArray<4, vigra::UInt8>();
    export_blockedArray<5, vigra::UInt8>();

    export_blockedArray<2, vigra::UInt32>();
    export_blockedArray<3, vigra::UInt32>();
    export_blockedArray<4, vigra::UInt32>();
    export_blockedArray<5, vigra::UInt32>();

    export_blockedArray<2, float>();
    export_blockedArray<3, float>();
    export_blockedArray<4, float>();
    export_blockedArray<5, float>();

}
