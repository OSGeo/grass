macro(set_alternate_linker linker)
  if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL
     "${USE_ALTERNATE_LINKER_OLD_CACHED}")
    unset(LINKER_EXECUTABLE CACHE)
  endif()
  find_program(LINKER_EXECUTABLE ld.${USE_ALTERNATE_LINKER}
               ${USE_ALTERNATE_LINKER})
  if(LINKER_EXECUTABLE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL 12.0.0)
        add_link_options("--ld-path=${LINKER_EXECUTABLE}")
      else()
        add_link_options("-fuse-ld=${LINKER_EXECUTABLE}")
      endif()
    elseif(
      "${linker}" STREQUAL "mold"
      AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
      AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 12.1.0)
      # GCC before 12.1.0: -fuse-ld does not accept mold as a valid argument, so
      # you need to use -B option instead.
      get_filename_component(_dir ${LINKER_EXECUTABLE} DIRECTORY)
      get_filename_component(_dir ${_dir} DIRECTORY)
      if(EXISTS "${_dir}/libexec/mold/ld")
        add_link_options(-B "${_dir}/libexec/mold")
      else()
        message(FATAL_ERROR "Cannot find ${_dir}/libexec/mold/ld")
      endif()
    else()
      add_link_options("-fuse-ld=${USE_ALTERNATE_LINKER}")
    endif()
    message(STATUS "Using alternative linker: ${LINKER_EXECUTABLE}")
  else()
    message(
      FATAL_ERROR "Cannot find alternative linker ${USE_ALTERNATE_LINKER}")
  endif()
endmacro()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}"
                                                  STREQUAL "GNU")
  set(USE_ALTERNATE_LINKER
      ""
      CACHE
        STRING
        "Use alternate linker. Leave empty for system default; potential alternatives are 'gold', 'lld', 'bfd', 'mold'"
  )
  if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL "")
    set_alternate_linker(${USE_ALTERNATE_LINKER})
  endif()
  set(USE_ALTERNATE_LINKER_OLD_CACHED
      ${USE_ALTERNATE_LINKER}
      CACHE INTERNAL "Previous value of USE_ALTERNATE_LINKER")
endif()
