project(blockedarray)

cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(Boost COMPONENTS python REQUIRED)
find_package(VIGRA REQUIRED)
find_package(HDF5 REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty ${VIGRA_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/snappy)
include_directories(${Boost_INCLUDE_DIR})

if(WIN32)
    find_package(NUMPY REQUIRED)
    include_directories(${NUMPY_INCLUDE_DIR})
endif()

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

add_subdirectory(thirdparty)
add_subdirectory(blockedarray)
