import numpy as np
import unittest

from adapters import ExampleSource, ExampleSink


class TestSource(unittest.TestCase):

    def setUp(self):
        self.vol = np.random.randint(0, 2**16-1, (100, 100, 10)).astype(np.uint32)

    def testExampleSource(self):
        vol = self.vol
        s = ExampleSource(vol)

        roi = [np.zeros((len(vol.shape),)), vol.shape]
        newVol = np.zeros(vol.shape, dtype=np.uint32)
        assert s.pyReadBlock(roi, newVol)
        assert np.all(vol == newVol)

    def testRoi(self):
        vol = self.vol
        s = ExampleSource(vol)

        reqRoi = [(50, 50, 2), (70, 70, 4)]
        s.pySetRoi(reqRoi)
        roi = [(0, 0, 0), (20, 20, 2)]
        newVol = np.zeros((20, 20, 2), dtype=np.uint32)
        assert s.pyReadBlock(roi, newVol)
        assert np.all(vol[50:70, 50:70, 2:4] == newVol)

    def testRoi2(self):
        vol = self.vol
        s = ExampleSource(vol)
        roi = [(0, 0, 2), (20, 20, 4)]
        newVol = np.zeros((20, 20, 2), dtype=np.uint32)
        assert s.pyReadBlock(roi, newVol)


class TestSink(unittest.TestCase):

    def setUp(self):
        pass

    def testExampleSink(self):
        s = ExampleSink()
        s.shape = (100, 100, 10)
        s.blockShape = (10, 10, 10)
        s.pyWriteBlock([(15, 20, 2), (30, 30, 6)],
                       np.ones((15, 10, 4), dtype=np.uint8))

        assert np.all(s.vol[15:30, 20:30, 2:6] == 1)
