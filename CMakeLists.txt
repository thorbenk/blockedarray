project(blockedarray)

cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(PythonLibs REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(VIGRA REQUIRED)
find_package(HDF5 REQUIRED)
find_package(Valgrind)
find_package(Snappy)

include(CheckCXXSourceCompiles)

option(BUILD_COMMON_DTYPES_LIBRARY "BuildCommonDtypesLibrary" OFF)

if(BUILD_COMMON_DTYPES_LIBRARY)
    CHECK_CXX_SOURCE_COMPILES(
        "template<class T> class Test {}; extern template class Test<int>; int main(int argc, char** argv) {};"
        HAS_EXTERN_TEMPLATES
    )
    if(HAS_EXTERN_TEMPLATES)
        add_definitions(-DHAS_EXTERN_TEMPLATES)
    endif()
endif()

if(NOT SNAPPY_FOUND)
    message(STATUS "using internal copy of snappy")
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty ${VIGRA_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/snappy)
    include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIR}
    )
else()
    message(STATUS "using snappy ${SNAPPY_LIBRARY}")
endif()

include_directories(
    ${VIGRA_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${HDF5_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(WIN32)
    find_package(NUMPY REQUIRED)
    include_directories(${NUMPY_INCLUDE_DIR})
endif()

add_subdirectory(src)
if(NOT SNAPPY_FOUND)
    add_subdirectory(thirdparty)
endif()
add_subdirectory(test)
if(VALGRIND_FOUND)
    add_subdirectory(bench)
endif()
add_subdirectory(blockedarray)
add_subdirectory(examples)
