set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs)
find_package(Boost COMPONENTS python REQUIRED)

#
# The _blockedarray python module
#

include_directories(
    ${PYTHON_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
)

add_library(_blockedarray SHARED
    module_py.cxx
    blockedarray_py.cxx
)
target_link_libraries(_blockedarray
    snappy
    ${PYTHON_LIBRARY}
    ${Boost_PYTHON_LIBRARIES}
    ${Boost_TIMER_LIBRARY}
    ${VIGRA_NUMPY_CORE_LIBRARY}
    ${VIGRA_NUMPY_IMPEX_LIBRARY}
    rt
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
