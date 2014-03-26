import numpy as np
import vigra

import unittest

from lazyflow.graph import Graph

from numpy.testing import assert_array_equal, assert_array_almost_equal

#import blockedarray.OpBlockedConnectedComponents
from blockedarray import OpBlockedConnectedComponents



class TestOpBlockedConnectedComponents(unittest.TestCase):

    def setUp(self):
        bg = np.zeros((1,1,1,1,1), dtype=np.uint8)
        bg = vigra.taggedView(bg, axistags='xyzct')
        self.bg = bg

    def testSimpleUsage(self):
        vol = np.random.randint(255, size=(1000, 100, 10))
        vol = vol.astype(np.uint8)
        vol = vigra.taggedView(vol, axistags='xyz')
        vol = vol.withAxes(*'xyzct')

        op = OpBlockedConnectedComponents(graph=Graph())
        op.Input.setValue(vol)
        op.Background.setValue(self.bg)

        out = op.Output[...].wait()

        assert_array_equal(vol.shape, out.shape)

    def testCorrectLabeling(self):
        vol = np.zeros((1000, 100, 10))
        vol = vol.astype(np.uint8)
        vol = vigra.taggedView(vol, axistags='xyz')

        vol[20:40, 10:30, 2:4] = 1
        vol = vol.withAxes(*'xyzct')

        op = OpBlockedConnectedComponents(graph=Graph())
        op.Input.setValue(vol)
        op.Background.setValue(self.bg)

        out = op.Output[...].wait()
        tags = op.Output.meta.getTaggedShape()
        out = vigra.taggedView(out, axistags=op.Output.meta.axistags)

        assert_array_equal(vol, out)

    def testCorrectLabelingInt(self):
        vol = np.zeros((1000, 100, 10))
        vol = vol.astype(np.uint32)
        vol = vigra.taggedView(vol, axistags='xyz')

        vol[20:40, 10:30, 2:4] = 1
        vol = vol.withAxes(*'xyzct')

        op = OpBlockedConnectedComponents(graph=Graph())
        op.Input.setValue(vol)
        op.Background.setValue(self.bg)

        out = op.Output[...].wait()
        tags = op.Output.meta.getTaggedShape()
        out = vigra.taggedView(out, axistags=op.Output.meta.axistags)

        assert_array_equal(vol, out)


class TestSimpleThings(unittest.TestCase):
    def testRoi(self):
        from blockedarray import dim3
        roi = dim3.Roi((0,0,0), (2,3,4))
        p = roi.p
        assert isinstance(p, tuple)
        assert len(p) ==3
        q = roi.q
        assert isinstance(q, tuple)
        assert len(q) ==3

        roi.p = q
        roi.q = p
        
        tempQ = roi.p
        assert isinstance(tempQ, tuple)
        assert tempQ == q


