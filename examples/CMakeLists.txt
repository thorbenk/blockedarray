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
