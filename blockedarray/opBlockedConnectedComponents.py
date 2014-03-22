
import numpy as np
import vigra

from lazyflow.operator import Operator, InputSlot, OutputSlot
from lazyflow.operators import OpCompressedCache
from lazyflow.operators.opLabelVolume import OpNonLazyCC
from lazyflow.rtype import SubRegion

from _blockedarray import dim2, dim3


class OpBlockedConnectedComponents(OpNonLazyCC):
    name = "OpBlockedConnectedComponents"
    supportedDtypes = [np.uint8, np.uint32]
    
    def __init__(self, *args, **kwargs):
        super(OpBlockedConnectedComponents, self).__init__(*args, **kwargs)
        self._sourceMap3 = dict(zip(self.supportedDtypes, 
                                    [dim3.PySourceU8, dim3.PySourceU32]))
        self._sinkMap3 = {np.uint32: dim3.PySinkU32}
        self._ccMap3 = dict(zip(self.supportedDtypes, 
                                [dim3.ConnectedComponentsU8,
                                 dim3.ConnectedComponentsU32]))

    def _updateSlice(self, c, t, bg):
        bg = self.Background[0, 0, 0, c, t].wait()
        assert bg == 0, "Non-zero background not supported"
        source = self._getSource(c, t)
        sink = self._getSink(c, t)
        #TODO enable 2d
        blockShape = tuple([int(s) for s in self._cache.BlockShape.value[:3]])
        CC = self._ccMap3[self.Input.meta.dtype]
        cc = CC(source, blockShape)

        cc.writeToSink(sink)

    ## factory for dynamic source creation
    def _getSource(self, c, t):
        SourceClass = self._sourceMap3[self.Input.meta.dtype]
        shape = tuple([int(s) for s in self.Input.meta.shape[:3]])
        
        class TempSource(SourceClass):
            def __init__(self, *args, **kwargs):
                self._slot = kwargs['slot']
                del kwargs['slot']
                super(TempSource, self).__init__(*args, **kwargs)
            def pySetRoi(self, roi):
                assert False, "Not supported"
            def pyShape(self):
                return shape
            def pyReadBlock(self, roi, block):
                start = roi.getP() + (c, t)
                stop = roi.getQ() + (c+1, t+1)
                subr = SubRegion(self._slot, start=start, stop=stop)
                block[:] = self._slot.get(subr).wait()[..., 0, 0]
                return True

        return TempSource(slot=self.Input)

    ## factory for dynamic sink creation
    def _getSink(self, c, t):
        SinkClass = self._sinkMap3[self.Output.meta.dtype]
        
        class TempSink(SinkClass):
            def __init__(self, *args, **kwargs):
                self._cache = kwargs['cache']
                del kwargs['cache']
                super(TempSink, self).__init__(*args, **kwargs)
            def pyWriteBlock(self, roi, block):
                block = vigra.taggedView(block, axistags='xyz')
                block = block.withAxes(*'xyzct')
                start = roi.getP() + (c, t)
                stop = roi.getQ() + (c+1, t+1)
                subr = SubRegion(self._cache.Input, start=start, stop=stop)
                self._cache.setInSlot(self._cache.Input, (), subr, block)
                return True

        return TempSink(cache=self._cache)


def printParents(obj, indent=0):
    def printIndented(s):
        print("{}{}".format(" "*indent, s))
    if type(obj) != type:
        print("Class hierarchy for {} (is a {})".format(obj, type(obj)))
        printParents(type(obj))
    else:
        if indent > 15:
            return
        printIndented(obj)
        for sub in obj.__bases__:
            printParents(sub, indent=indent+1)

