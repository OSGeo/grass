find_path(SQLITE_INCLUDE_DIR sqlite3.h)
find_library(SQLITE_LIBRARY NAMES sqlite3)

mark_as_advanced(SQLITE_LIBRARY)
mark_as_advanced(SQLITE_INCLUDE_DIR)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(SQLite REQUIRED_VARS SQLITE_LIBRARY
                                                       SQLITE_INCLUDE_DIR)
