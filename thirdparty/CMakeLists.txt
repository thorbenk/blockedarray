if (WIN32)
    set(ac_cv_have_stdint_h "1\ntypedef ptrdiff_t ssize_t;")
    set(ac_cv_have_stddef_h 1)
    set(ac_cv_have_sys_uio_h 0)
else()
    set(ac_cv_have_stdint_h 1)
    set(ac_cv_have_stddef_h 1)
    set(ac_cv_have_sys_uio_h 1)
endif()

configure_file(snappy/snappy-stubs-public.h.in ${CMAKE_CURRENT_BINARY_DIR}/snappy-stubs-public.h @ONLY)

set(SNAPPY_SRCS
    snappy/snappy.cc
    snappy/snappy-c.cc
    snappy/snappy-sinksource.cc
    snappy/snappy-stubs-internal.cc
)

if(WIN32)
    add_library(snappy STATIC ${SNAPPY_SRCS})
else()
    add_library(snappy SHARED ${SNAPPY_SRCS})
endif()

