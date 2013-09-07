import numpy
import vigra
import h5py

from _blockedarray import *

def rw(ba):
    f = h5py.File("test_ba_py.h5", 'w')
    ba.writeHDF5(f.fid.id, "ba")
    f.close()
    
    #f = h5py.File("test_ba_py.h5", 'w')
    #ba2.readHDF5(f.fid.id, "ba")
    #f.close()

def test1():
    blockShape = (20,30,40)
    ba = BlockedArray3uint8(blockShape)
    rw(ba)
    assert ba.numBlocks() == 0
    assert ba.sizeBytes() == 0

    p = (5,6,8)
    q = (40,50,60)
    a = numpy.ones(tuple([b-a for a,b in zip(p,q)]), dtype=numpy.uint8)
    a *= 42
    b = numpy.zeros((10,10,10), dtype=numpy.uint8)

    ba.writeSubarray(p, q, a)
    assert ba.numBlocks() > 0
    assert ba.sizeBytes() > 0

    a_read = numpy.zeros(a.shape, a.dtype)
    ba.readSubarray(p, q, a_read)

    assert numpy.all(a_read == a)

    sl = numpy.s_[5:40, 6:50, 8:60]

    ba[sl] = numpy.ones(a.shape, a.dtype)

    assert numpy.all(ba[sl] == 1)
   
    ba.deleteSubarray((0,0,0), (100,100,100))
    assert ba.numBlocks() == 0
    assert ba.sizeBytes() == 0
    
    ba.setMinMaxTrackingEnabled(True)
    
    ba[3:5,6:7,8:20] = 42*numpy.ones((2,1,12), dtype=numpy.uint8)
    ba[1:2,1:2,1:2]  = 2*numpy.ones((1,1,1), dtype=numpy.uint8)
    
    assert ba.minMax() == (0,42) #TODO: do we want the zero there?
    
    ba.deleteSubarray((0,0,0), (100,100,100))
    assert ba.numBlocks() == 0
    assert ba.sizeBytes() == 0
   
    ba.setManageCoordinateLists(True)
   
    ba[1:2,3:4,7:8]  = 5*numpy.ones((1,1,1), dtype=numpy.uint8)
    
    vv = ba.nonzero()
    
    coords = vv[0:3]
    values = vv[3]
    
    assert coords[0][0] == 1
    assert coords[1][0] == 3
    assert coords[2][0] == 7
    assert values[0]    == 5 
    
    ba.setCompressionEnabled(True)
    ba.setCompressionEnabled(False)

if __name__ == "__main__":
    test1()
