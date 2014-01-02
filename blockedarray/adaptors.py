
import numpy
from _blockedarray import Source3U8 as Source


class SourceABC(Source):
    def __init__(self):
        super(SourceABC, self).__init__()

    '''
     * selects only the region of interest given from the
     * underlying data source. When readBlock() is used, the coordinates
     * are relative to roi.q
    '''
    def setRoi(self, roi):
        raise NotImplementedError
        #pass

    def shape(self):
        raise NotImplementedError
        #return (10, 10, 10)

    def readBlock(self, roi, output):
        raise NotImplementedError
        #return True


class TestSource(SourceABC):
    def setRoi(self, roi):
        pass

    def shape(self):
        return numpy.asarray((100, 100, 10), dtype=numpy.long)

    def readBlock(self, roi, output):
        output[...] = 0
        return True
