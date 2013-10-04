from _blockedarray import *
import numpy
import vigra
import time
import copy

from lazyflow.operators import OpBlockedArrayCache, OpArrayPiper, OpSlicedBlockedArrayCache
from lazyflow.graph import Graph
from lazyflow.operators.opCxxBlockedArrayCache import OpCxxBlockedArrayCache

def generateSlicings(n=100, twoD = False):
    sl = []
    while(True):
        start = numpy.zeros(len(dataShape), numpy.uint32)
        stop  = numpy.zeros(len(dataShape), numpy.uint32)
        for i in range(len(dataShape)):
            start[i] = copy.copy(numpy.random.randint(low=0, high=dataShape[i])) # inclusive, exclusive
            stop[i] = copy.copy(numpy.random.randint(low=start[i]+1, high=dataShape[i]+1))
            assert stop[i] >= start[i]+1
            assert stop[i] < dataShape[i]+1
        assert numpy.all(stop > start), "start = %r, stop=%r" % (start, stop)
        assert numpy.all(stop <= dataShape)
        assert numpy.all(start >= 0)
        assert numpy.all(stop >= 0)

        slicing = tuple([slice(a,b) for a,b in zip(start, stop)])
        
        if twoD:
            x = numpy.random.randint(1,4)
            slicing = list(slicing)
            slicing[x] = slice(slicing[x].start, slicing[x].start+1)
            slicing = tuple(slicing)
        
        sl.append(slicing)
        if(len(sl) >= n):
            break
    return sl

if __name__ == "__main__":
    numpy.random.seed(0)
    
    dataShape =  (1,200,200,200,1)
    blockShape = (1,32 ,32, 32, 1)
    outerBlockShape = blockShape
    
    sl = generateSlicings(100, twoD=True)
    for slicing in sl:
        print slicing

    g = Graph()

    ba = BlockedArray5uint8(blockShape)

    print "generate random data"
    data = (255*numpy.random.random(dataShape)).astype(numpy.uint8)

    data = data.view(vigra.VigraArray)
    data.axistags = vigra.defaultAxistags('txyzc')

    graph = Graph()

    opProvider = OpArrayPiper(graph=graph)
    opProvider.Input.setValue(data)
    opProvider = opProvider

    #python cache
    opCache = OpBlockedArrayCache(graph=graph)
    opCache.Input.meta.shape = dataShape
    opCache.Input.connect(opProvider.Output)
    opCache.innerBlockShape.setValue(blockShape)
    opCache.outerBlockShape.setValue(outerBlockShape)
    opCache.fixAtCurrent.setValue(False)

    #python sliced array cache
    opCacheSliced = OpSlicedBlockedArrayCache(graph=graph)
    opCacheSliced.Input.meta.shape = dataShape
    opCacheSliced.Input.connect(opProvider.Output)
   
    #these are the actual values in pixel classification
    #blockDimsX = { 't' : (1,1), 'z' : (128,256), 'y' : (128,256), 'x' : (1,1), 'c' : (100, 100) }
    #blockDimsY = { 't' : (1,1), 'z' : (128,256), 'y' : (1,1), 'x' : (128,256), 'c' : (100,100) }
    #blockDimsZ = { 't' : (1,1), 'z' : (1,1), 'y' : (128,256), 'x' : (128,256), 'c' : (100,100) }
    
    #blockDimsX = { 't' : (1,1), 'z' : (32,32), 'y' : (32,32), 'x' : (1,1), 'c' : (32,32) }
    #blockDimsY = { 't' : (1,1), 'z' : (32,32), 'y' : (1,1), 'x' : (32,32), 'c' : (32,32) }
    #blockDimsZ = { 't' : (1,1), 'z' : (1,1), 'y' : (32,32), 'x' : (32,32), 'c' : (32,32) }
    
    blockDimsX = { 't' : (1,1), 'z' : (32,32), 'y' : (32,32), 'x' : (32,32), 'c' : (32,32) }
    blockDimsY = { 't' : (1,1), 'z' : (32,32), 'y' : (32,32), 'x' : (32,32), 'c' : (32,32) }
    blockDimsZ = { 't' : (1,1), 'z' : (32,32), 'y' : (32,32), 'x' : (32,32), 'c' : (32,32) }
    
    axisOrder=['t', 'x', 'y', 'z', 'c']
    innerBlockShapeX = tuple( blockDimsX[k][0] for k in axisOrder )
    outerBlockShapeX = tuple( blockDimsX[k][1] for k in axisOrder )
    innerBlockShapeY = tuple( blockDimsY[k][0] for k in axisOrder )
    outerBlockShapeY = tuple( blockDimsY[k][1] for k in axisOrder )
    innerBlockShapeZ = tuple( blockDimsZ[k][0] for k in axisOrder )
    outerBlockShapeZ = tuple( blockDimsZ[k][1] for k in axisOrder )
    
    opCacheSliced.innerBlockShape.setValue( (innerBlockShapeX, innerBlockShapeY, innerBlockShapeZ) )
    opCacheSliced.outerBlockShape.setValue( (outerBlockShapeX, outerBlockShapeY, outerBlockShapeZ) )
    
    opCacheSliced.fixAtCurrent.setValue(False)

    #C++ cache
    opCacheCpp = OpCxxBlockedArrayCache(graph=graph)
    opCacheCpp.Input.connect(opProvider.Output)
    opCacheCpp.innerBlockShape.setValue(blockShape)
    opCacheCpp.outerBlockShape.setValue(blockShape)

    #make sure that all blocks are in the cache

    print "request all from cpp",
    t = time.time()
    opCacheCpp.Output[tuple([slice(0, dataShape[i]) for i in range(len(dataShape))])].wait()
    #opCacheCpp.fixAtCurrent.setValue(True)
    print " took ", time.time()-t

    print "request all from py (array cached)",
    t = time.time()
    opCache.Output[tuple([slice(0, dataShape[i]) for i in range(len(dataShape))])].wait()
    #opCache.fixAtCurrent.setValue(True)
    print " took ", time.time()-t

    print "request all from py (sliced array cached)",
    t = time.time()
    opCacheSliced.Output[tuple([slice(0, dataShape[i]) for i in range(len(dataShape))])].wait()
    #opCacheSliced.fixAtCurrent.setValue(True)
    print " took ", time.time()-t

    slicings = []
    iter = 0

    ts = []
    ts2 = []

    #sl = [ (slice(0, 1, None), slice(77, 93, None), slice(56, 71, None), slice(138, 145, None), slice(0, 1, None)) ]
    
    #sl = [ (slice(0, 1, None), slice(48, 156, None), slice(113, 114, None), slice(188, 200, None), slice(0, 1, None)) ]
    #sl  = [ (slice(0, 1, None), slice(174, 192, None), slice(193, 200, None), slice(10, 11, None), slice(0, 1, None)) ]
    #sl = [ (slice(0, 1, None), slice(58, 95, None), slice(10, 97, None), slice(43, 44, None), slice(0, 1, None)) ]
    
    for slicing in sl:
        dataCachedSlice = None
        dataCpp         = None
        dataPy          = None
       
        print "size: %f^3" % numpy.power(numpy.prod([s.stop-s.start for s in slicing]), 1/3.0), "slicing =", slicing
        
        t0 = time.time()
        dataCacheSliced = opCacheSliced.Output( slicing ).wait() 
        tPySliced = time.time()-t0
        if tPySliced*1000.0 > 10:
            print "&&&&&&&&&&&&&&&&&&&&&&&&&&&&"
        print "py1: %f msec." % (tPySliced*1000.0,)

        t1 = time.time()
        dataPy = opCache.Output( slicing ).wait() 
        tPy = time.time()-t1
        print "py2: %f msec." % (tPy*1000.0,)

        t2 = time.time()
        dataCpp = opCacheCpp.Output( slicing ).wait() 
        tCpp = time.time()-t2
        print "cpp  %f msec." % (tCpp*1000.0,)
        
        ts.append( tCpp/float(tPy) ) 
        ts2.append( tCpp/float(tPySliced) ) 

        assert numpy.all(dataPy == dataCpp)
        if dataPy is not None and dataCachedSlice is not None:
            assert numpy.all(dataPy == dataCacheSliced)

        iter += 1

    print "average ratio: cpp/py:         ", numpy.average(ts)
    print "average ratio: cpp/py(sliced): ", numpy.average(ts2)

