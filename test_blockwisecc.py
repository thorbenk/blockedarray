import numpy
import vigra
import h5py
from blockedarray import dim2 
from blockedarray import dim3

def test2():
    img = vigra.impex.readImage("balls.jpg").view(numpy.ndarray).mean(axis=2)
    print img.min(), img.max(), img.mean()

    vigra.writeHDF5(img, "/tmp/img.h5", "img")

    bwt = dim2.BlockwiseThresholding("/tmp/img.h5", "img", dim2.V(100,100))
    bwt.run(100, "/tmp/thresh.h5", "thresh")

    t = vigra.readHDF5("/tmp/thresh.h5", "thresh")
    vigra.impex.writeImage(255*t, "/tmp/thethresh.png")

    provider = dim2.HDF5BlockProvider("/tmp/thresh.h5", "thresh")

    bwc = dim2.BlockwiseConnectedComponents(provider, dim2.V(100,100))
    bwc.writeResult("/tmp/theresult.h5", "cc")

    l = vigra.readHDF5("/tmp/theresult.h5", "cc")
    print "xxx ", l.min(), l.max()
    ctable = (255.0*numpy.random.random((l.max()+1, 3))).astype(numpy.uint8)
    lcol = ctable[l]
    vigra.impex.writeImage(lcol, "/tmp/theresult.png")

    import sys; sys.exit()
    
def test3():
    img = vigra.impex.readImage("balls.jpg").view(numpy.ndarray).mean(axis=2)
    print img.min(), img.max(), img.mean()
    img = numpy.dstack([img]*50)

    vigra.writeHDF5(img, "/tmp/img.h5", "img")

    bwt = dim3.BlockwiseThresholding("/tmp/img.h5", "img", dim3.V(10,100,100))
    bwt.run(100, "/tmp/thresh.h5", "thresh")

    t = vigra.readHDF5("/tmp/thresh.h5", "thresh")
    vigra.impex.writeImage(255*t[:,:,25], "/tmp/thethresh.png")

    provider = dim3.HDF5BlockProvider("/tmp/thresh.h5", "thresh")

    bwc = dim3.BlockwiseConnectedComponents(provider, dim3.V(10,100,100))
    bwc.writeResult("/tmp/theresult.h5", "cc")

    l = vigra.readHDF5("/tmp/theresult.h5", "cc")
    l = l[:,:,25]
    ctable = (255.0*numpy.random.random((l.max()+1, 3))).astype(numpy.uint8)
    lcol = ctable[l]
    vigra.impex.writeImage(lcol, "/tmp/theresult.png")

    import sys; sys.exit()

test3()
#test2()
