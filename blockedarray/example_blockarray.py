from _blockedarray import *
import numpy
import vigra

blockShape = (20,30,40)
ba = BlockedArray3uint8(blockShape)

p = (5,6,8)
q = (40,50,60)
a = numpy.ones(tuple([b-a for a,b in zip(p,q)]), dtype=numpy.uint8)
a *= 42

print a.shape, a.dtype, type(a)

b = numpy.zeros((10,10,10), dtype=numpy.uint8)

ba.writeSubarray(p, q, a)

a_read = numpy.zeros(a.shape, a.dtype)
ba.readSubarray(p, q, a_read)

print a_read

print ba[5:40, 6:50, 8:60]

ba[5:40, 6:50, 8:60] = numpy.ones(a.shape, a.dtype)

print ba[5:40, 6:50, 8:60]
