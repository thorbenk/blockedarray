blockedarray
============

[![Build Status](https://travis-ci.org/thorbenk/blockedarray.png?branch=master)](https://travis-ci.org/thorbenk/blockedarray)

C++ code and boost::python wrapper for a blocked,
in-memory compressed array that supports read/write
access to arbitrary regions of interest.

compressed array
----------------

The class `CompressedArray<N,T>` represents a `N`-dimensional
array with pixel type `T`. The array data can be stored
compressed (`CompressedArray::compress()`) or uncompressed
(`CompressedArray::uncompress()`). When reading data
(`CompressedArray::readArray`), the data is - if needed -
uncompressed first.

The compression algorithm used is google
[snappy](https://code.google.com/p/snappy).

blocked array
-------------

`BlockedArray<N,T>` stores `N`-dimension array data
of pixel type `T` in blocks
(of a size to be specified in the constructor).
Each block is stored compressed in memory, using
google snappy, a fast compression/decompression algorithm.

Regions of interest of arbitrary offset and shape can be
read, written and deleted.

This class is useful if the whole dataset does not fit
into memory. It is the intention that this class is evaluated
as a replacement for
[lazyflow](http://github.com/ilastik/lazyflow)'s
`OpBlockedArrayCache`, `OpSlicedBlockArrayCache`,
`OpCompressedCache`.
Towards this goal, a preliminary operator wrapper can be found
in `blockedarray/lazyflow`.

installation
------------
- Dependencies: boost headers and boost python library, cmake.  
  Ubuntu: `sudo apt-get install libboost-dev libboost-python-dev cmake`
- Install latest vigra from https://github.com/ukoethe/vigra  
  Ubuntu:
      git clone https://github.com/ukoethe/vigra
      mkdir build && cd build
      cmake -DCMAKE_INSTALL_PREFIX=<install prefix> ..
      make install

Finally, to install `blockedarray`:

```bash
mkdir build
cd build
cmake ..
git submodule init
git submodule update
make
```
