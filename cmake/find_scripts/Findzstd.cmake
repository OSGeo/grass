find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(zstd QUIET "libzstd")
  set(zstd_LIBRARIES ${zstd_LINK_LIBRARIES})
else()
  find_path(zstd_ROOT_DIR NAMES include/zstd.h)
  find_path(
    zstd_INCLUDE_DIRS
    NAMES zstd.h
    HINTS ${zstd_ROOT_DIR}/include)
  find_library(
    zstd_LIBRARIES
    NAMES zstd
    HINTS ${zstd_ROOT_DIR}/lib)
  if(zstd_INCLUDE_DIRS AND zstd_LIBRARIES)
    set(zstd_FOUND TRUE)
  endif()
endif()

if(zstd_FOUND)
  message(STATUS "Found zstd: ${zstd_LIBRARIES}")
endif()
