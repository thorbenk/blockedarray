ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) 

add_executable(ccpipeline ccpipeline.cpp)
target_link_libraries(ccpipeline
    ${VIGRA_IMPEX_LIBRARY}
    ${HDF5_LIBRARY}
    ${HDF5_HL_LIBRARY}
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(ccpipeline bw)
else()
    target_link_libraries(ccpipeline snappy)
endif()

set(EXTRACTMESH_SRCS "extractmesh.cpp")
if(NOT BUILD_COMMON_DTYPES_LIBRARY)
    list(APPEND EXTRACTMESH_SRCS ${PROJECT_SOURCE_DIR}/src/meshextractor.cpp)
endif()

add_executable(extractmesh ${EXTRACTMESH_SRCS})
target_link_libraries(extractmesh
    ${VIGRA_IMPEX_LIBRARY}
    ${HDF5_LIBRARY}
    ${HDF5_HL_LIBRARY}
    
)
if(BUILD_COMMON_DTYPES_LIBRARY)
    target_link_libraries(extractmesh bw)
else()
    target_link_libraries(extractmesh snappy)
endif()

find_package(VTK)
if(VTK_FOUND)
include(${VTK_USE_FILE})

add_executable(showmesh showmesh.cpp)
target_link_libraries(showmesh vtkHybrid ${VIGRA_IMPEX_LIBRARY} ${HDF5_HL_LIBRARY} ${HDF5_LIBRARY})

endif()