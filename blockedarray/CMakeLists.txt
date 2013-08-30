set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs REQUIRED)
find_package(PythonInterp REQUIRED)
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

if(WIN32 OR APPLE)
    set(RT_LIBRARY "")
else()
    set(RT_LIBRARY "rt")
endif()

target_link_libraries(_blockedarray
    snappy
    ${PYTHON_LIBRARY}
    ${Boost_PYTHON_LIBRARIES}
    ${HDF5_LIBRARY}
    ${HDF5_HL_LIBRARY}
    ${VIGRA_IMPEX_LIBRARY}
    ${RT_LIBRARY}
)
IF(WIN32)                                                                                                               
    MESSAGE(STATUS "WIN32")                                                                                             
    SET_TARGET_PROPERTIES(_blockedarray PROPERTIES OUTPUT_NAME "_blockedarray" PREFIX "" SUFFIX  ".pyd")                                                                                                                                                                                                                                                                                   
ELSEIF(APPLE)                                                                                                           
    MESSAGE(STATUS "APPLE")                                                                                             
    SET_TARGET_PROPERTIES(_blockedarray PROPERTIES OUTPUT_NAME "_blockedarray" PREFIX "" SUFFIX ".so")                                      
ELSE()                                                                                                                  
    MESSAGE(STATUS "LINUX")                                                                                             
    SET_TARGET_PROPERTIES(_blockedarray PROPERTIES OUTPUT_NAME "_blockedarray" PREFIX "")                                                   
ENDIF()

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
