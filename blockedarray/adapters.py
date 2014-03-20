
import vigra

import numpy as np
from _blockedarray import Source3U8 as _Source
from _blockedarray import Sink3 as _Sink

## Source Interface
#
# This class provides the python interface to the C++ class `BW::Source`. If
# you inherit from this class, be sure to implement *all* abstract methods.
class SourceABC(_Source):

    ## Constructor
    #
    # Be sure to call super().__init__ if you override the constructor!
    def __init__(self):
        super(SourceABC, self).__init__()

    ## Set a custom ROI
    # selects only the region of interest given from the
    # underlying data source. When readBlock() is used, the coordinates
    # are relative to roi[0]
    #
    # @param roi a tuple containing 2 lists of length 3 (incl. start, excl. stop)
    def pySetRoi(self, roi):
        raise NotImplementedError
        #pass

    ## Volume shape getter
    #
    # @return must be a tuple of (python) integers
    def pyShape(self):
        raise NotImplementedError
        #return (10, 10, 10)

    ## Block getter
    #
    # read a block of data into a 3d numpy array 
    #
    # @param roi a tuple containing 2 lists of length 3 (incl. start, excl. stop)
    # @param output a 3d numpy.ndarray with shape roi[1]-roi[0] and dtype uint8
    def pyReadBlock(self, roi, output):
        raise NotImplementedError
        #return True


## Sink Interface
#
# This class provides the python interface to the C++ class `BW::Sink`. If
# you inherit from this class, be sure to implement *all* abstract methods.
class SinkABC(_Sink):

    ## Constructor
    #
    # Be sure to call super().__init__ if you override the constructor!
    def __init__(self):
        super(SinkABC, self).__init__()

    ## Write a block of data
    #
    # @param roi a tuple containing 2 lists of length 3 (incl. start, excl. stop)
    # @param block a 3d numpy.ndarray with shape roi[1]-roi[0] and dtype int32
    def pyWriteBlock(self, roi, block):
        raise NotImplementedError
        #return True


class DummySource(SourceABC):
    def __init__(self):
        super(DummySource, self).__init__()

    def pySetRoi(self, roi):
        pass

    def pyShape(self):
        return (100, 100, 10)

    def pyReadBlock(self, roi, output):
        output[...] = 0
        return True


class ExampleSource(SourceABC):
    def __init__(self, vol):
        # the call to super() is super-important!
        super(ExampleSource, self).__init__()
        self._vol = vol
        self._p = np.zeros((len(vol.shape),), dtype=np.long)
        self._q = np.asarray(vol.shape, dtype=np.long)

    def pySetRoi(self, roi):
        assert len(roi) == 2
        self._p = np.asarray(roi[0], dtype=np.long)
        self._q = np.asarray(roi[1], dtype=np.long)

    def pyShape(self):
        return self._vol.shape

    def pyReadBlock(self, roi, output):
        assert len(roi) == 2
        roiP = np.asarray(roi[0])
        roiQ = np.asarray(roi[1])
        p = self._p + roiP
        q = p + roiQ - roiP
        if np.any(q > self._q):
            raise IndexError("Requested roi is too large for selected "
                             "roi (previous call to setRoi)")
        s = _roi2slice(p, q)
        output[...] = self._vol[s]
        return True


class ExampleSink(SinkABC):
    def __init__(self):
        super(ExampleSink, self).__init__()
        self.vol = None

    def pyWriteBlock(self, roi, block):
        assert len(roi) == 2
        if self.vol is None:
            shape = self.shape
            shape = _v2tup(shape)
            self.vol = np.zeros(shape, dtype=np.uint8)
        s = _roi2slice(roi[0], roi[1])
        self.vol[s] = block


def _v2tup(v, d=3):
    return tuple([v[i] for i in range(d)])


def _roi2slice(p, q):
    s = [slice(p[i], q[i]) for i in range(len(p))]
    return tuple(s)
