find_path(SQLITE_INCLUDE_DIR sqlite3.h)
find_library(SQLITE_LIBRARY NAMES sqlite3 )

mark_as_advanced(SQLITE_LIBRARY)
mark_as_advanced(SQLITE_INCLUDE_DIR)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLITE  REQUIRED_VARS SQLITE_LIBRARY SQLITE_INCLUDE_DIR)
