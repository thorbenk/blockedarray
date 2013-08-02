ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) 

add_executable(ccpipeline ccpipeline.cpp)
target_link_libraries(ccpipeline
    snappy
    ${VIGRA_IMPEX_LIBRARY}
    ${HDF5_LIBRARY}
    ${HDF5_HL_LIBRARY}
)
