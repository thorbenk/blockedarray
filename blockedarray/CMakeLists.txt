set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs REQUIRED)
find_package(PythonInterp REQUIRED)
find_package(Numpy REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)

if(BUILD_COMMON_DTYPES_LIBRARY)
    set(EXTRA_SRCS " ")
else()
    set(EXTRA_SRCS ${PROJECT_SOURCE_DIR}/src/hdf5utils.cxx)
endif()

#
# The _blockedarray python module
#

include_directories(
    ${PYTHON_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PYTHON_NUMPY_INCLUDE_DIR}
)

add_library(_blockedarray SHARED
    module_py.cxx
    blockedarray_py.cxx
    blockwisecc_py.cxx
    ${EXTRA_SRCS}
)

if(WIN32 OR APPLE)
    set(RT_LIBRARY "")
else()
    set(RT_LIBRARY "rt")
endif()

target_link_libraries(_blockedarray
    ${Boost_PYTHON_LIBRARIES}
    snappy
    ${PYTHON_LIBRARY}
    ${HDF5_LIBRARY}
    ${HDF5_HL_LIBRARY}
    ${VIGRA_IMPEX_LIBRARY}
    ${RT_LIBRARY}
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(_blockedarray bw)
endif()

if(WIN32)                                                                                                               
    set_target_properties(_blockedarray PROPERTIES OUTPUT_NAME "_blockedarray" PREFIX "" SUFFIX  ".pyd")
elseif(APPLE)
    set_target_properties(_blockedarray PROPERTIES OUTPUT_NAME "_blockedarray" PREFIX "" SUFFIX ".so")
else()
    set_target_properties(_blockedarray PROPERTIES OUTPUT_NAME "_blockedarray" PREFIX "")
endif()

get_property(location TARGET _blockedarray PROPERTY LOCATION)
add_custom_command(TARGET _blockedarray
    POST_BUILD        
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  
        ${location}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME test_py_blockedarray
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
         COMMAND ${PYTHON_EXECUTABLE} test_blockedarray.py)
