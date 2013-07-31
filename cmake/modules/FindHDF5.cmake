# This module finds an installed HDF5 package.
#
# It sets the following variables:
#  HDF5_FOUND         - set to false, or undefined, if hdf5 isn't found.
#  HDF5_INCLUDE_DIR   - the hdf5 include directory.
#  HDF5_LIBRARY       - the hdf5 library

FIND_PATH(HDF5_INCLUDE_DIR hdf5.h)
FIND_LIBRARY(HDF5_LIBRARY NAMES hdf5)
FIND_LIBRARY(HDF5_HL_LIBRARY NAMES hdf5_hl)

IF(WIN32 AND HDF5_LIBRARY MATCHES "dll.lib$")
    SET(HDF5_CFLAGS "-D_HDF5USEDLL_")
    SET(HDF5_CPPFLAGS "-D_HDF5USEDLL_ -DHDF5CPP_USEDLL")
ELSE()
    SET(HDF5_CFLAGS)
    SET(HDF5_CPPFLAGS)
ENDIF()

IF (HDF5_INCLUDE_DIR AND HDF5_LIBRARY AND HDF5_HL_LIBRARY)
    SET(HDF5_FOUND TRUE)
ENDIF ()

IF (HDF5_FOUND)
    # show which HDF5 was found only if not quiet
    IF (NOT HDF5_FIND_QUIETLY)
      MESSAGE(STATUS "Found hdf5")
      MESSAGE(STATUS "  > library    : ${HDF5_LIBRARY}")
      MESSAGE(STATUS "  > include dir: ${HDF5_INCLUDE_DIR}")
    ENDIF (NOT HDF5_FIND_QUIETLY)
ELSE (HDF5_FOUND)
    # fatal error if HDF5 is required but not found
    IF (HDF5_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find HDF5")
    ENDIF (HDF5_FIND_REQUIRED)
ENDIF (HDF5_FOUND)
