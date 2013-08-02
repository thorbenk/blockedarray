ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) 

add_executable(test_blockedarray test_blockedarray.cpp)
target_link_libraries(test_blockedarray
    snappy
    ${VIGRA_IMPEX_LIBRARY}
)
add_test("test_blockedarray" test_blockedarray)

add_executable(test_compressedarray test_compressedarray.cpp)
target_link_libraries(test_compressedarray
    snappy
)
add_test("test_compressedarray" test_compressedarray)

add_executable(test_blockedcc test_blockedcc.cpp)
target_link_libraries(test_blockedcc
    snappy
)
add_test("test_blockedcc" test_blockedcc)


add_executable(test_roi test_roi.cpp)
add_test("test_roi" test_roi)

add_executable(test_blocking test_blocking.cpp)
add_test("test_blocking" test_blocking)

add_executable(test_hdf5blockedsource test_hdf5blockedsource.cpp)
target_link_libraries(test_hdf5blockedsource ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
add_test("test_hdf5blockedsource" test_hdf5blockedsource)

add_executable(test_hdf5blockedsink test_hdf5blockedsink.cpp)
target_link_libraries(test_hdf5blockedsink ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
add_test("test_hdf5blockedsink" test_hdf5blockedsink)

add_executable(test_blockwisethresholding test_blockwisethresholding.cpp)
target_link_libraries(test_blockwisethresholding ${VIGRA_IMPEX_LIBRARY} ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
add_test("test_blockwisethresholding" test_blockwisethresholding)
