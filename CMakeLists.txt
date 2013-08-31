project(blockedarray)

cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(PythonLibs REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(VIGRA REQUIRED)
find_package(HDF5 REQUIRED)

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

include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty ${VIGRA_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/snappy)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIR}
)

if(WIN32)
    find_package(NUMPY REQUIRED)
    include_directories(${NUMPY_INCLUDE_DIR})
endif()

add_subdirectory(src)
add_subdirectory(thirdparty)
add_subdirectory(test)
add_subdirectory(blockedarray)
add_subdirectory(examples)
