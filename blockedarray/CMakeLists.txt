set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs)
find_package(Numpy REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)

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
)

if(WIN32 or APPLE)
    set(RT_LIBRARY "")
else()
    set(RT_LIBRARY "rt")
endif()

target_link_libraries(_blockedarray
    snappy
    ${Boost_PYTHON_LIBRARIES}
    ${HDF5_LIBRARY}
    ${HDF5_HL_LIBRARY}
    ${VIGRA_IMPEX_LIBRARY}
    ${RT_LIBRARY}
)
set_target_properties(_blockedarray PROPERTIES
    PREFIX ""
)
get_property(location TARGET _blockedarray PROPERTY LOCATION)
add_custom_command(TARGET _blockedarray
    POST_BUILD        
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  
        ${location}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
