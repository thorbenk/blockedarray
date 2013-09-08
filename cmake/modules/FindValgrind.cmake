INCLUDE(FindPackageHandleStandardArgs)

FIND_PATH(VALGRIND_INCLUDE_DIR
  NAMES valgrind/callgrind.h
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Valgrind "Could NOT find valgrind includes. See http://valgrind.org" VALGRIND_INCLUDE_DIR)

