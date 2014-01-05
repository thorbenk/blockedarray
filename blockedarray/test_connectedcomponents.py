import numpy as np
import unittest

from adapters import DummySource, ExampleSource
from _blockedarray import dim3


class TestConnectedComponents(unittest.TestCase):

    def setUp(self):
        shape = (100, 100, 100)
        self.blockShape = (10, 10, 10)
        vol = np.zeros(shape, dtype=np.uint8)
        vol[7:12, 7:12, 7:12] = 42
        vol[:, :, 72:75] = 111
        vol[:, 72:75, :] = 111
        self.vol = vol

    def testBlockShape(self):
        CC = dim3.ConnectedComponents
        s = DummySource()

        v = (10, 10, 10)
        cc = CC(s, v)

        v = np.asarray((10, 10, 10))
        cc = CC(s, v)

        v = np.asarray((10, 10, 10), dtype=np.long)
        cc = CC(s, v)

        v = np.asarray((10, 10, 10), dtype=np.int)
        cc = CC(s, v)

    def testCC(self):
        CC = dim3.ConnectedComponents
        s = ExampleSource(self.vol)
        assert CC(s, self.blockShape)
