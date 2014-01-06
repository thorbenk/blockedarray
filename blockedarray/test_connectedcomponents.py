import unittest
import tempfile
import os

import numpy as np
from numpy.testing import assert_almost_equal
import vigra

from adapters import DummySource, ExampleSource, ExampleSink
from _blockedarray import dim3


CC = dim3.ConnectedComponents

class TestConnectedComponents(unittest.TestCase):

    def setUp(self):
        shape = (100, 110, 120)
        self.blockShape = (10, 10, 10)
        vol = np.zeros(shape, dtype=np.uint8)
        vol[7:12, 17:22, 27:32] = 42
        vol[:, :, 72:75] = 111
        vol[:, 72:75, :] = 111
        self.vol = vol

    def testBlockShape(self):
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
        tmp = tempfile.NamedTemporaryFile(suffix='.h5', delete=False)
        try:
            h5f = (tmp.name, "/data")
            s = ExampleSource(self.vol)
            cc = CC(s, self.blockShape)

            cc.writeResult(h5f[0], h5f[1])
            labels = vigra.readHDF5(h5f[0], h5f[1])
        finally:
            pass
            os.remove(h5f[0])

        # vigra and hdf5 have incompatible axis order, see
        # http://ukoethe.github.io/vigra/doc/vigra/classvigra_1_1HDF5File.html#a638be2d51070009b02fed0f234f4b0bf
        labels = np.transpose(labels, (2, 1, 0))

        self._check(labels)

    def testCCsink(self):
        source = ExampleSource(self.vol)
        sink = ExampleSink()
        cc = CC(source, self.blockShape)
        cc.writeToSink(sink)

        self._check(sink.vol)

    def _check(self, labels):
        comp1 = np.concatenate((labels[:, :, 72:75].flatten(),
                                labels[:, 72:75, :].flatten()))
        comp2 = labels[7:12, 17:22, 27:32].flatten()

        # assert that each component has exactly one positive label
        assert_almost_equal(comp1.var(), 0)
        assert np.all(comp1 > 0)
        assert_almost_equal(comp2.var(), 0)
        assert np.all(comp2 > 0)

        # assert that each component is labeled with a unique label
        assert comp1[0] != comp2[0]

        # assert that no other pixel is labeled
        labels[:, :, 72:75] = 0
        labels[:, 72:75, :] = 0
        labels[7:12, 17:22, 27:32] = 0
        assert_almost_equal(labels.sum(), 0)

