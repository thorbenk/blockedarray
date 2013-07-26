#Python
import time
import weakref
import itertools
from threading import Lock
import logging
logger = logging.getLogger(__name__)
from functools import partial

#SciPy
import numpy

#lazyflow
from lazyflow.request import RequestPool
from lazyflow.drtile import drtile
from lazyflow.roi import sliceToRoi, roiToSlice, getBlockBounds, TinyVector
from lazyflow.graph import InputSlot, OutputSlot
from lazyflow.utility import fastWhere, Tracer
from lazyflow.operators.opCache import OpCache
from lazyflow.operators.opArrayPiper import OpArrayPiper
from lazyflow.operators.arrayCacheMemoryMgr import ArrayCacheMemoryMgr, MemInfoNode

from blockedarray import *

class OpArrayCacheCpp(OpCache):
    """ Allocates a block of memory as large as Input.meta.shape (==Output.meta.shape)
        with the same dtype in order to be able to cache results.
        
        blockShape: dirty regions are tracked with a granularity of blockShape
    """
    
    name = "ArrayCache"
    description = "numpy.ndarray caching class"
    category = "misc"

    DefaultBlockSize = 64

    #Input
    Input = InputSlot()
    blockShape = InputSlot(value = DefaultBlockSize)
    fixAtCurrent = InputSlot(value = False)
   
    #Output
    CleanBlocks = OutputSlot()
    Output = OutputSlot()

    loggingName = __name__ + ".OpArrayCache"
    logger = logging.getLogger(loggingName)
    traceLogger = logging.getLogger("TRACE." + loggingName)

    # Block states
    IN_PROCESS  = 0
    DIRTY       = 1
    CLEAN       = 2
    FIXED_DIRTY = 3

    def __init__(self, *args, **kwargs):
        super( OpArrayCacheCpp, self ).__init__(*args, **kwargs)
        self._origBlockShape = self.DefaultBlockSize
        
        self._last_access = None
        self._blockShape = None
        self._fixed = False
        self._lock = Lock()
        self._cacheHits = 0
        self._has_fixed_dirty_blocks = False
        self._memory_manager = ArrayCacheMemoryMgr.instance
        self._running = 0

    def usedMemory(self):
        pass
    
    #def usedMemory(self):
    #    if self._cache is not None:
    #        return self._cache.nbytes
    #    else:
    #        return 0

    #def _blockShapeForIndex(self, index):
    #    if self._cache is None:
    #        return None
    #    cacheShape = numpy.array(self._cache.shape)
    #    blockStart = index * self._blockShape
    #    blockStop = numpy.minimum(blockStart + self._blockShape, cacheShape)
        
    def fractionOfUsedMemoryDirty(self):
        pass
    
    #def fractionOfUsedMemoryDirty(self):
    #    totAll   = numpy.prod(self.Output.meta.shape)
    #    totDirty = 0
    #    for i, v in enumerate(self._blockState.ravel()):
    #        sh = self._blockShapeForIndex(i)
    #        if sh is None:
    #            continue
    #        if v == self.DIRTY or v == self.FIXED_DIRTY:
    #            totDirty += numpy.prod(sh)
    #    return totDirty/float(totAll)
    
    def lastAccessTime(self):
        return self._last_access
    
    def generateReport(self, report):
        pass
    
    #def generateReport(self, report):
    #    report.name = self.name
    #    report.fractionOfUsedMemoryDirty = self.fractionOfUsedMemoryDirty()
    #    report.usedMemory = self.usedMemory()
    #    report.lastAccessTime = self.lastAccessTime()
    #    report.dtype = self.Output.meta.dtype
    #    report.type = type(self)
    #    report.id = id(self)

    def _freeMemory(self, refcheck = True):
        pass
    
    #def _freeMemory(self, refcheck = True):
    #    with self._cacheLock:
    #        freed  = self.usedMemory()
    #        if self._cache is not None:
    #            fshape = self._cache.shape
    #            try:
    #                self._cache.resize((1,), refcheck = refcheck)
    #            except ValueError:
    #                freed = 0
    #                self.logger.warn("OpArrayCache: freeing failed due to view references")
    #            if freed > 0:
    #                self.logger.debug("OpArrayCache: freed cache of shape:{}".format(fshape))
    #
    #                self._lock.acquire()
    #                self._blockState[:] = OpArrayCache.DIRTY
    #                del self._cache
    #                self._cache = None
    #                self._lock.release()
    #        return freed

    def _allocateManagementStructures(self):
        pass
    
    #def _allocateManagementStructures(self):
    #    with Tracer(self.traceLogger):
    #        shape = self.Output.meta.shape
    #        if type(self._origBlockShape) != tuple:
    #            self._blockShape = (self._origBlockShape,)*len(shape)
    #        else:
    #            self._blockShape = self._origBlockShape
    #
    #        self._blockShape = numpy.minimum(self._blockShape, shape)
    #
    #        self._dirtyShape = numpy.ceil(1.0 * numpy.array(shape) / numpy.array(self._blockShape))
    #
    #        self.logger.debug("Configured OpArrayCache with shape={}, blockShape={}, dirtyShape={}, origBlockShape={}".format(shape, self._blockShape, self._dirtyShape, self._origBlockShape))
    #
    #        #if a request has been submitted to get a block, the request object
    #        #is stored within this array
    #        self._blockQuery = numpy.ndarray(self._dirtyShape, dtype=object)
    #       
    #        #keep track of the dirty state of each block
    #        self._blockState = OpArrayCache.DIRTY * numpy.ones(self._dirtyShape, numpy.uint8)
    #
    #        self._blockState[:]= OpArrayCache.DIRTY
    #        self._dirtyState = OpArrayCache.CLEAN
    
    #def _allocateCache(self):
    #    with self._cacheLock:
    #        self._last_access = None
    #        self._cache_priority = 0
    #        self._running = 0
    #
    #        if self._cache is None or (self._cache.shape != self.Output.meta.shape):
    #            mem = numpy.zeros(self.Output.meta.shape, dtype = self.Output.meta.dtype)
    #            self.logger.debug("OpArrayCache: Allocating cache (size: %dbytes)" % mem.nbytes)
    #            if self._blockState is None:
    #                self._allocateManagementStructures()
    #            self._cache = mem
    #    self._memory_manager.add(self)

    def setupOutputs(self):
        self.CleanBlocks.meta.shape = (1,)
        self.CleanBlocks.meta.dtype = object
        
        reconfigure = False
        if  self.inputs["fixAtCurrent"].ready():
            self._fixed =  self.inputs["fixAtCurrent"].value

        if self.inputs["blockShape"].ready() and self.inputs["Input"].ready():
            newBShape = self.inputs["blockShape"].value
            if not isinstance(newBShape, tuple):
                newBShape = tuple([int(newBShape)]*len(self.Input.meta.shape))
            if self._origBlockShape != newBShape and self.inputs["Input"].ready():
                reconfigure = True
            self._origBlockShape = newBShape
            self._blockShape = newBShape

           
            inputSlot = self.inputs["Input"]
            self.outputs["Output"].meta.assignFrom(inputSlot.meta)

        shape = self.outputs["Output"].meta.shape

        if reconfigure and shape is not None:
            self._lock.acquire()
            if self.Input.meta.dtype == numpy.uint32:
                t = "uint32"
            elif self.Input.meta.dtype == numpy.int32: 
                t = "int32"
            elif self.Input.meta.dtype == numpy.float32: 
                t = "float32"
            elif self.Input.meta.dtype == numpy.int64: 
                t = "int64"
            elif self.Input.meta.dtype == numpy.uint8:
                t = "uint8"
            else:
                raise RuntimeError("dtype %r not supported" % self.Input.meta.dtype)
            cls = "BlockedArray%d%s" % (len(self._blockShape), t)
            self.b = eval(cls)(self._blockShape)
            self.b.setDirty(tuple([0]*len(self._blockShape)), self.Input.meta.shape, True)
            
            self._lock.release()

    def propagateDirty(self, slot, subindex, roi):
        shape = self.Output.meta.shape
        
        key = roi.toSlice()
        
        if slot == self.inputs["Input"]:
            with self._lock:
                self.b.setDirty(roi.start, roi.stop, True)
            if not self._fixed:
                self.outputs["Output"].setDirty(key)
                
        if slot == self.inputs["fixAtCurrent"]:
            if self.inputs["fixAtCurrent"].ready():
                self._fixed = self.inputs["fixAtCurrent"].value
                if not self._fixed and self.Output.meta.shape is not None and self._has_fixed_dirty_blocks:
                    pass
                    '''
                    # We've become unfixed, so we need to notify downstream 
                    #  operators of every block that became dirty while we were fixed.
                    # Convert all FIXED_DIRTY states into DIRTY states
                    with self._lock:
                        dirtyBlocks = self.b.dirtyBlocks(000, self.Output.meta.shape)
                        newDirtyBlocks = diff(dirtyBlocks, oldDityBlocks)
                        self._has_fixed_dirty_blocks = False
                    newDirtyBlocks = numpy.transpose(numpy.nonzero(cond))
                    
                    # To avoid lots of setDirty notifications, we simply merge all the dirtyblocks into one single superblock.
                    # This should be the best option in most cases, but could be bad in some cases.
                    # TODO: Optimize this by merging the dirty blocks via connected components or something.
                    cacheShape = numpy.array(self.Output.meta.shape)
                    dirtyStart = cacheShape
                    dirtyStop = [0] * len(cacheShape)
                    for index in newDirtyBlocks:
                        blockStart = index * self._blockShape
                        blockStop = numpy.minimum(blockStart + self._blockShape, cacheShape)
                        
                        dirtyStart = numpy.minimum(dirtyStart, blockStart)
                        dirtyStop = numpy.maximum(dirtyStop, blockStop)

                    if len(newDirtyBlocks > 0):
                        self.Output.setDirty( dirtyStart, dirtyStop )
                    '''

    def _updatePriority(self, new_access = None):
        pass
    
    #def _updatePriority(self, new_access = None):
    #    if self._last_access is None:
    #        self._last_access = new_access or time.time()
    #    cur_time = time.time()
    #    delta = cur_time - self._last_access + 1e-9
    #
    #    self._last_access = cur_time
    #    new_prio = 0.5 * self._cache_priority + delta
    #    self._cache_priority = new_prio

    def execute(self, slot, subindex, roi, result):
        if slot == self.Output:
            return self._executeOutput(slot, subindex, roi, result)
        elif slot == self.CleanBlocks:
            pass #FIXME
            #return self._executeCleanBlocks(slot, subindex, roi, result)
        
    def _executeOutput(self, slot, subindex, roi, result):
        key = roi.toSlice()

        shape = self.Output.meta.shape
        start, stop = sliceToRoi(key, shape)

        self.traceLogger.debug("Acquiring ArrayCache lock...")
        self._lock.acquire()
        self.traceLogger.debug("ArrayCache lock acquired.")

        ch = self._cacheHits
        ch += 1
        self._cacheHits = ch

        self._running += 1

        bp, bq = self.b.dirtyBlocks(start, stop)
        
        #print "there are %d dirty blocks" % bp.shape[0]

        if not self._fixed:
            reqs = []
            sh = self.outputs["Output"].meta.shape
            for i in range(bp.shape[0]):
                bStart = tuple([int(t) for t in bp[i,:]])
                bStop  = tuple([int(t) for t in numpy.minimum(bq[i,:], sh)])
                key = roiToSlice(bStart, bStop)
                req = self.Input[key]
                reqs.append((req, bStart, bStop))
            for r, bStart, bStop in reqs:
                r.wait()
            for r, bStart, bStop in reqs:
                x = r.wait()
                self.b.writeSubarray(bStart, bStop, r.wait())
        
        t1 = time.time()
        self.b.readSubarray(start, stop, result)
        
        #print "read subarray took %f" % (time.time()-t1)
            
        self._lock.release()
        
        return result

    def setInSlot(self, slot, subindex, roi, value):
        print "SET IN SLOT &&&&&"
        assert slot == self.inputs["Input"]
        ch = self._cacheHits
        ch += 1
        self._cacheHits = ch
        start, stop = roi.start, roi.stop
        
        self._lock.acquire()
        print "***", start, stop, value.shape
        self.b.writeSubarray(start, stop, value)
        self._lock.release()

    #def _executeCleanBlocks(self, slot, subindex, roi, destination):
    #    indexCols = numpy.where(self._blockState == OpArrayCache.CLEAN)
    #    clean_block_starts = numpy.array(indexCols).transpose()
    #        
    #    inputShape = self.Input.meta.shape
    #    clean_block_rois = map( partial( getBlockBounds, inputShape, self._blockShape ),
    #                            clean_block_starts )
    #    destination[0] = map( partial(map, TinyVector), clean_block_rois )
    #    return destination
