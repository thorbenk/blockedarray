
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
    def pySetRoi(self, roi):
        raise NotImplementedError
        #pass

    def pyShape(self):
        raise NotImplementedError
        #return (10, 10, 10)

    def pyReadBlock(self, roi, output):
        raise NotImplementedError
        #return True


class TestSource(SourceABC):
    def __init__(self):
        super(TestSource, self).__init__()

    def pySetRoi(self, roi):
        pass

    def pyShape(self):
        return numpy.asarray((100, 100, 10), dtype=numpy.long)

    def pyReadBlock(self, roi, output):
        output[...] = 0
        return True
