#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    This is the main function that builds all grass libraries (prefixed with grass_)
            and grass executables. This cmake function is tailored to meet requirement of grass
            gnu make rules
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

include(GenerateExportHeader)
function(build_module)
  cmake_parse_arguments(
    G
    "EXE;NO_DOCS"
    "NAME;SRC_DIR;SRC_REGEX;RUNTIME_OUTPUT_DIR;PACKAGE;HTML_FILE_NAME"
    "SOURCES;INCLUDES;DEPENDS;OPTIONAL_DEPENDS;PRIMARY_DEPENDS;DEFS;HEADERS;TEST_SOURCES"
    ${ARGN})

  if(NOT G_NAME)
    message(FATAL_ERROR "G_NAME empty")
  endif()

  foreach(PRIMARY_DEPEND ${G_PRIMARY_DEPENDS})
    if(NOT TARGET ${PRIMARY_DEPEND})
      message(
        STATUS "${G_NAME} disabled because ${PRIMARY_DEPEND} is not available")
      return()
    else()
      list(APPEND G_DEPENDS ${PRIMARY_DEPEND})
    endif()
  endforeach()

  if(NOT G_SRC_REGEX)
    set(G_SRC_REGEX "*.[ch]")
  endif()

  if(NOT G_SRC_DIR)
    set(G_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  set(html_file "${G_SRC_DIR}/${G_NAME}.html")

  foreach(G_HEADER ${G_HEADERS})
    if(EXISTS "${G_SRC_DIR}/${G_HEADER}")
      file(COPY ${G_SRC_DIR}/${G_HEADER}
           DESTINATION "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass")
    else()
      file(
        GLOB header_list_from_glob
        LIST_DIRECTORIES false
        "${G_SRC_DIR}/${G_HEADER}")
      if(NOT header_list_from_glob)
        message(
          FATAL_ERROR
            "MUST copy '${G_SRC_DIR}/${G_HEADER}' to ${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass"
        )
      endif()
      foreach(header_I ${header_list_from_glob})
        file(COPY ${header_I}
             DESTINATION "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/grass")
      endforeach()
    endif()
  endforeach()

  if(NOT G_SOURCES)
    file(GLOB ${G_NAME}_SRCS "${G_SRC_DIR}/${G_SRC_REGEX}")
  else()
    set(${G_NAME}_SRCS ${G_SOURCES})
  endif()

  set(RUN_HTML_DESCR TRUE)
  # Auto set if to run RUN_HTML_DESCR
  if(G_EXE)
    set(RUN_HTML_DESCR TRUE)
    if(G_RUNTIME_OUTPUT_DIR)
      set(RUN_HTML_DESCR FALSE)
    endif()
    # g.parser and some others does not have --html-description.
    if(${G_NAME} IN_LIST NO_HTML_DESCR_TARGETS)
      set(RUN_HTML_DESCR FALSE)
    endif()
  else()
    set(RUN_HTML_DESCR FALSE)
  endif()

  set(install_dest "")
  if(NOT G_RUNTIME_OUTPUT_DIR)
    if(G_EXE)
      set(G_RUNTIME_OUTPUT_DIR "${OUTDIR}/${GRASS_INSTALL_BINDIR}")
      set(install_dest "${GRASS_INSTALL_BINDIR}")
    else()
      set(G_RUNTIME_OUTPUT_DIR "${OUTDIR}/${GRASS_INSTALL_LIBDIR}")
      set(install_dest "${GRASS_INSTALL_LIBDIR}")
    endif()
  else()
    set(install_dest "${G_RUNTIME_OUTPUT_DIR}")
    set(G_RUNTIME_OUTPUT_DIR "${OUTDIR}/${install_dest}")
  endif()

  if(MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${G_RUNTIME_OUTPUT_DIR}>)
  elseif(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${G_RUNTIME_OUTPUT_DIR})
  endif()

  if(G_EXE)
    add_executable(${G_NAME} ${${G_NAME}_SRCS})
    if("${G_NAME}" MATCHES "^v[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Vector)
    elseif("${G_NAME}" MATCHES "^r[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Raster)
    elseif("${G_NAME}" MATCHES "^d[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Display)
    elseif("${G_NAME}" MATCHES "^db[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Database)
    elseif("${G_NAME}" MATCHES "^g[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/General)
    elseif("${G_NAME}" MATCHES "^i[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Imagery)
    elseif("${G_NAME}" MATCHES "^m[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Miscellaneous)
    elseif("${G_NAME}" MATCHES "^ps[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/PostScript)
    elseif("${G_NAME}" MATCHES "^r3[\.]")
      set_target_properties(${G_NAME} PROPERTIES FOLDER "Tools/Raster 3D")
    elseif("${G_NAME}" MATCHES "^t[\.]")
       set_target_properties(${G_NAME} PROPERTIES FOLDER Tools/Temporal)
    else()
      set_target_properties(${G_NAME} PROPERTIES FOLDER Binaries)
    endif()
    set(default_html_file_name ${G_NAME})
    set(PGM_NAME ${G_NAME})
    if(WIN32)
      set(PGM_NAME ${G_NAME}.exe)
    endif()

    set(modules_list
        "${G_NAME};${modules_list}"
        CACHE INTERNAL "list of modules")

  else()
    string(REPLACE "grass_" "" _libname ${G_NAME})
    add_library(${G_NAME} ${${G_NAME}_SRCS})
    set_target_properties(
      ${G_NAME}
      PROPERTIES FOLDER "GRASS Libraries"
                 VERSION ${GRASS_VERSION_NUMBER}
                 SOVERSION ${GRASS_VERSION_MAJOR}
                 EXPORT_NAME ${_libname})

    add_library(GRASS::${_libname} ALIAS ${G_NAME})

    # TODO: check when and where the export header files are needed
    set(export_file_name
        "${OUTDIR}/${GRASS_INSTALL_INCLUDEDIR}/export/${G_NAME}_export.h")
    # Default is to use library target name without grass_ prefix
    string(REPLACE "grass_" "" default_html_file_name ${G_NAME})
    set(PGM_NAME ${default_html_file_name})

    generate_export_header(${G_NAME} STATIC_DEFINE "STATIC_BUILD"
                           EXPORT_FILE_NAME ${export_file_name})
    add_dependencies(${G_NAME} python_doc_utils)
  endif()

  if(G_HTML_FILE_NAME)
    set(HTML_FILE_NAME ${G_HTML_FILE_NAME})
  else()
    set(HTML_FILE_NAME ${default_html_file_name})
  endif()

  get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
  set_property(GLOBAL PROPERTY MODULE_LIST "${MODULE_LIST};${G_NAME}")

  add_dependencies(${G_NAME} INCLUDE_HEADERS)

  foreach(G_OPTIONAL_DEPEND ${G_OPTIONAL_DEPENDS})
    if(TARGET ${G_OPTIONAL_DEPEND})
      add_dependencies(${G_NAME} ${G_OPTIONAL_DEPEND})
    endif()
  endforeach()
  foreach(G_DEPEND ${G_DEPENDS})
    if(NOT TARGET ${G_DEPEND})
      message(FATAL_ERROR "${G_DEPEND} not a target")
      break()
    endif()

    add_dependencies(${G_NAME} ${G_DEPEND})

    set(${G_NAME}_INCLUDE_DIRS)
    list(APPEND ${G_NAME}_INCLUDE_DIRS "${G_SRC_DIR}")
    foreach(G_INCLUDE ${G_INCLUDES})
      list(APPEND ${G_NAME}_INCLUDE_DIRS "${G_INCLUDE}")
    endforeach()

    if(${G_NAME}_INCLUDE_DIRS)
      list(REMOVE_DUPLICATES ${G_NAME}_INCLUDE_DIRS)
    endif()
    target_include_directories(
      ${G_NAME} PUBLIC "$<BUILD_INTERFACE:${${G_NAME}_INCLUDE_DIRS}>")
  endforeach()

  foreach(G_DEF ${G_DEFS})
    target_compile_definitions(${G_NAME} PUBLIC "${G_DEF}")
  endforeach()

  set(package_define)
  if(NOT G_PACKAGE)
    if(G_EXE)
      set(package_define "grassmods")
    else()
      set(package_define "grasslibs")
    endif()
  else()
    if(NOT G_PACKAGE STREQUAL "NONE")
      set(package_define ${G_PACKAGE})
    endif()
  endif()

  target_compile_definitions(${G_NAME}
                             PRIVATE "-DPACKAGE=\"${package_define}\"")

  foreach(dep ${G_DEPENDS} ${G_OPTIONAL_DEPENDS})
    if(TARGET ${dep})
      get_target_property(interface_def ${dep} INTERFACE_COMPILE_DEFINITIONS)
      if(interface_def)
        target_compile_definitions(${G_NAME} PRIVATE "${interface_def}")
      endif()
      target_link_libraries(${G_NAME} PRIVATE ${dep})
    endif()
  endforeach()

  set(G_HTML_FILE_NAME "${HTML_FILE_NAME}.html")
  set(html_file_search ${G_SRC_DIR}/${G_HTML_FILE_NAME})
  if(NOT G_NO_DOCS AND NOT EXISTS ${html_file_search})
    set(G_NO_DOCS YES)
  endif()

  if(WITH_DOCS AND NOT G_NO_DOCS)

    set(html_file)
    if(EXISTS ${html_file_search})
      set(html_file ${html_file_search})
    endif()

    if(NOT html_file)
      return()
    endif()

    get_filename_component(HTML_FILE_NAME ${html_file} NAME)
    get_filename_component(PGM_SOURCE_DIR ${html_file} PATH)

    string(REPLACE ".html" "" PGM_NAME "${HTML_FILE_NAME}")

    if(RUN_HTML_DESCR)
      set(HTML_DESCR "HTML_DESCR")
    endif()

    generate_docs(${PGM_NAME}
      TARGET ${G_NAME}
      SOURCEDIR ${PGM_SOURCE_DIR}
      ${HTML_DESCR})

  endif() # WITH_DOCS

  foreach(test_SOURCE ${G_TEST_SOURCES})
    add_test(NAME ${G_NAME}-test
             COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE}
                     ${G_SRC_DIR}/testsuite/${test_SOURCE})
    message("[build_module] ADDING TEST ${G_NAME}-test")
  endforeach()

  if(NOT G_EXE)
    string(REPLACE "grass_" "" _libname ${G_NAME})
    install(
      TARGETS ${G_NAME}
      EXPORT ${_libname}Targets
      LIBRARY DESTINATION ${install_dest}
      ARCHIVE DESTINATION ${install_dest}
      RUNTIME DESTINATION ${G_RUNTIME_OUTPUT_DIR}
      INCLUDES
      DESTINATION ${GRASS_INSTALL_INCLUDEDIR})

    install(
      EXPORT ${_libname}Targets
      FILE GRASS_${_libname}Targets.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GRASS
      NAMESPACE GRASS::
      EXPORT_LINK_INTERFACE_LIBRARIES)
  else()
    install(TARGETS ${G_NAME} DESTINATION ${install_dest})
  endif()

  set(_headers ${G_HEADERS})
  list(TRANSFORM _headers PREPEND "${G_SRC_DIR}/")
  file(GLOB _docs_files
       LIST_DIRECTORIES FALSE
       ${G_SRC_DIR}/*.html ${G_SRC_DIR}/*.md
       ${G_SRC_DIR}/*.png ${G_SRC_DIR}/*.jpg)
  target_sources(${G_NAME} PRIVATE ${_headers} ${_docs_files})
endfunction()
