ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) 

add_executable(test_blockedarray test_blockedarray.cpp)
target_link_libraries(test_blockedarray
    snappy
    ${VIGRA_IMPEX_LIBRARY}
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_blockedarray bw)
endif()
target_link_libraries(test_blockedarray ${HDF5_LIBRARY})
add_test("test_blockedarray" test_blockedarray)

add_executable(test_compressedarray test_compressedarray.cpp)
target_link_libraries(test_compressedarray
    snappy
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_compressedarray bw)
endif()
target_link_libraries(test_compressedarray ${HDF5_LIBRARY})
add_test("test_compressedarray" test_compressedarray)

add_executable(test_blockedcc test_blockedcc.cpp)
target_link_libraries(test_blockedcc
    snappy
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_blockedcc bw)
endif()
add_test("test_blockedcc" test_blockedcc)


add_executable(test_roi test_roi.cpp)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_roi bw)
endif()
add_test("test_roi" test_roi)

add_executable(test_blocking test_blocking.cpp)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_blocking bw)
endif()
add_test("test_blocking" test_blocking)

add_executable(test_hdf5blockedsource test_hdf5blockedsource.cpp)
target_link_libraries(test_hdf5blockedsource ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_hdf5blockedsource bw)
endif()
add_test("test_hdf5blockedsource" test_hdf5blockedsource)

find_package(Boost COMPONENTS system filesystem)
if(Boost_SYSTEM_FOUND AND Boost_FILESYSTEM_FOUND)
    add_executable(test_sourceknossos test_sourceknossos.cpp)
    target_link_libraries(test_sourceknossos ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
    target_link_libraries(test_sourceknossos ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
    if(BUILD_COMMON_DTYPES_LIBRARY)
        target_link_libraries(test_sourceknossos bw)
    endif()
    add_test("test_sourceknossos" test_sourceknossos)
endif()

add_executable(test_hdf5blockedsink test_hdf5blockedsink.cpp)
target_link_libraries(test_hdf5blockedsink ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_hdf5blockedsink bw)
endif()
add_test("test_hdf5blockedsink" test_hdf5blockedsink)

add_executable(test_blockwisethresholding test_blockwisethresholding.cpp)
target_link_libraries(test_blockwisethresholding ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_blockwisethresholding bw)
endif()
add_test("test_blockwisethresholding" test_blockwisethresholding)

add_executable(test_blockwiseregionfeatures test_regionfeatures.cpp)
target_link_libraries(test_blockwiseregionfeatures ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_blockwiseregionfeatures bw)
endif()
add_test("test_blockwiseregionfeatures" test_blockwiseregionfeatures)

add_executable(test_blockwisechannelselector test_blockwisechannelselector.cpp)
target_link_libraries(test_blockwisechannelselector ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(test_blockwisechannelselector bw)
endif()
add_test("test_blockwisechannelselector" test_blockwisechannelselector)

