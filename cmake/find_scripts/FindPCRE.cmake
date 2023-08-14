find_path(PCRE_INCLUDE_DIR NAMES pcre.h)

find_library(PCRE_LIBRARY_RELEASE NAMES pcre)
find_library(PCRE_LIBRARY_DEBUG NAMES pcred)
if(PCRE_LIBRARY_DEBUG)
  set(PCRE_LIBRARY ${PCRE_LIBRARY_DEBUG})
elseif(PCRE_LIBRARY_RELEASE)
  set(PCRE_LIBRARY ${PCRE_LIBRARY_RELEASE})
endif()

set(PCRE_FOUND FALSE)
if(PCRE_INCLUDE_DIR AND PCRE_LIBRARY)
  set(PCRE_FOUND TRUE)
endif()

if(PCRE_FOUND)
  set(PCRE_LIBRARIES ${PCRE_LIBRARY})
  set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})
endif()

mark_as_advanced(PCRE_LIBRARY)
mark_as_advanced(PCRE_LIBRARY_DEBUG)
mark_as_advanced(PCRE_LIBRARY_RELEASE)
mark_as_advanced(PCRE_INCLUDE_DIR)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_LIBRARY
                                  PCRE_INCLUDE_DIR)
