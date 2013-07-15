set(ac_cv_have_stdint_h 1)
set(ac_cv_have_stddef_h 1)
set(ac_cv_have_sys_uio_h 1)

configure_file(snappy/snappy-stubs-public.h.in ${CMAKE_CURRENT_BINARY_DIR}/snappy-stubs-public.h @ONLY)

add_library(snappy
    snappy/snappy.cc
    snappy/snappy-c.cc
    snappy/snappy-sinksource.cc
    snappy/snappy-stubs-internal.cc
)
