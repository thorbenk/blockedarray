ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) 

include_directories(${PROJECT_SOURCE_DIR}/test)
include_directories(${VALGRIND_INCLUDE_DIR})

if(BUILD_COMMON_DTYPES_LIBRARY)
    set(EXTRA_SRCS " ")
else()
    set(EXTRA_SRCS ${PROJECT_SOURCE_DIR}/src/hdf5utils.cxx)
endif()

add_definitions(-g)

add_executable(bench_blockedarray bench_blockedarray.cpp ${EXTRA_SRCS})
target_link_libraries(bench_blockedarray
    snappy
    ${VIGRA_IMPEX_LIBRARY}
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(bench_blockedarray bw)
endif()
target_link_libraries(bench_blockedarray ${HDF5_LIBRARY})

add_executable(bench_compressedarray bench_compressedarray.cpp ${EXTRA_SRCS})
target_link_libraries(bench_compressedarray
    snappy
    ${VIGRA_IMPEX_LIBRARY}
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(bench_compressedarray bw)
endif()
target_link_libraries(bench_compressedarray ${HDF5_LIBRARY})

