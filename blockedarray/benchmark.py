from _blockedarray import *
import numpy
import vigra
import time
import copy

from lazyflow.operators import OpBlockedArrayCache, OpArrayPiper, OpSlicedBlockedArrayCache
from lazyflow.graph import Graph
from lazyflow.operators.opCxxBlockedArrayCache import OpCxxBlockedArrayCache

def generateSlicings(dataShape, n=100, twoD = False):
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

class Benchmark(object):
    def __init__(self, data, blockShape):
        numpy.random.seed(0)
        
        self.data = data
        self.blockShape = blockShape
        
        outerBlockShape = blockShape
        ba = BlockedArray5uint8(blockShape)

        print "* setup:"

        self.graph = Graph()

        self.opProvider = OpArrayPiper(graph=self.graph)
        self.opProvider.Input.setValue(self.data)

        #python cache
        self.opCachePy = OpBlockedArrayCache(graph=self.graph)
        self.opCachePy.Input.meta.shape = dataShape
        self.opCachePy.Input.connect(self.opProvider.Output)
        self.opCachePy.innerBlockShape.setValue(blockShape)
        self.opCachePy.outerBlockShape.setValue(outerBlockShape)
        self.opCachePy.fixAtCurrent.setValue(False)

        #C++ cache
        self.opCacheCpp = OpCxxBlockedArrayCache(graph=self.graph)
        self.opCacheCpp.Input.connect(self.opProvider.Output)
        self.opCacheCpp.innerBlockShape.setValue(blockShape)
        self.opCacheCpp.outerBlockShape.setValue(blockShape)

        #make sure that all blocks are in the cache

        print "  request all from cpp",
        self.tSetupCpp = time.time()
        self.opCacheCpp.Output[tuple([slice(0, self.data.shape[i]) for i in range(len(self.data.shape))])].wait()
        #opCacheCpp.fixAtCurrent.setValue(True)
        self.tSetupCpp = time.time() - self.tSetupCpp
        print " ... took ", self.tSetupCpp

        print "  request all from py ",
        self.tSetupPy = time.time()
        self.opCachePy.Output[tuple([slice(0, self.data.shape[i]) for i in range(len(self.data.shape))])].wait()
        #opCache.fixAtCurrent.setValue(True)
        self.tSetupPy = time.time() - self.tSetupPy
        print " ... took ", self.tSetupPy
        
    def runSlicings(self, slicings, assertData=False):
        tCpp = []
        tPy  = []
        for slicing in slicings:
            dataCpp         = None
            dataPy          = None
            #print "size: %f^3" % numpy.power(numpy.prod([s.stop-s.start for s in slicing]), 1/3.0), "slicing =", slicing
        
            tCpp.append(time.time())
            dataCpp = self.opCacheCpp.Output( slicing ).wait() 
            tCpp[-1] = time.time()-tCpp[-1]
            
            tPy.append(time.time())
            dataPy = self.opCachePy.Output( slicing ).wait() 
            tPy[-1] = time.time()-tPy[-1]
            
            if assertData:
                assert numpy.all(dataPy == dataCpp)
      
        tPy  = numpy.asarray(tPy, dtype=numpy.float64)
        tCpp = numpy.asarray(tCpp, dtype=numpy.float64)
        return tPy, tCpp


if __name__ == "__main__":
    print "* pre-setup:"
    print "  generate random data"
    
    dataShape =  (1,400,400,400,1)
    if True:
        data = (255*numpy.random.random(dataShape)).astype(numpy.uint8)
        data = data.view(vigra.VigraArray)
        data.axistags = vigra.defaultAxistags('txyzc')
        
        sl2 = generateSlicings(data.shape, 100, twoD=True) 
    
        l = numpy.linspace(30, 400, 50)
        
        res = numpy.zeros((len(l), 6))
        
        for i, x in enumerate(l):
            x = int(x)
            print "x =", x
            blockShape = (1,x,x,x,1)
    
            b = Benchmark(data, blockShape)
        
            tPy, tCpp = b.runSlicings(sl2)
            
            res[i,0] = x                       #block side length
            res[i,1] = numpy.average(tPy)      #average time to request (python)
            res[i,2] = numpy.average(tCpp)     #average time to request (C++)
            res[i,3] = numpy.average(tPy/tCpp) #ratio python time / c++ time for requests
            res[i,4] = b.tSetupPy
            res[i,5] = b.tSetupCpp
            print "py: %f  c++ %f" % (numpy.average(tPy), numpy.average(tCpp))
        
        numpy.savetxt("ba_bench.csv", res)
   
    res = numpy.loadtxt("ba_bench.csv")
    if res.ndim == 1:
        res = res.reshape((1, len(res)))
    
    ### plot the benchmark results
    
    from matplotlib import pyplot as plot
    
    plot.clf()
    plot.title("read requests\ncache for %r data" % (dataShape,))
    plot.plot(res[:,0], res[:,1], label="time py")
    plot.plot(res[:,0], res[:,2], label="time c++")
    plot.xlabel("block side length")
    plot.ylabel("seconds")
    plot.legend()
    plot.savefig("01_read.png")
    
    plot.clf()
    plot.title("read requests\ncache for %r data" % (dataShape,))
    plot.plot(res[:,0], res[:,3], label="Py/C++")
    plot.xlabel("block side length")
    plot.legend()
    plot.savefig("02_read.png")
    
    plot.clf()
    plot.title("setup time\ncache for %r data" % (dataShape,))
    plot.plot(res[:,0], res[:,4], label="Py")
    plot.plot(res[:,0], res[:,5], label="C++")
    plot.xlabel("block side length")
    plot.legend()
    plot.savefig("03_setup.png")

