project(blockedarray)

cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(Boost COMPONENTS system filesystem timer unit_test_framework)
find_package(VIGRA REQUIRED)
find_package(HDF5 REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty ${VIGRA_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/snappy)

add_executable(test_blockedarray test_blockedarray.cpp)
target_link_libraries(test_blockedarray
    stdc++
    snappy
    ${VIGRA_IMPEX_LIBRARY}
    ${HDF5_CORE_LIBRARY}
    ${HDF5_HL_LIBRARY}
    ${Boost_TIMER_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)
add_test("test_blockedarray" test_blockedarray)

add_executable(test_compressedarray test_compressedarray.cpp)
target_link_libraries(test_compressedarray
    snappy
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)
add_test("test_compressedarray" test_compressedarray)

add_subdirectory(thirdparty)
add_subdirectory(blockedarray)
