#[[
AUTHOR(S):  Nicklas Larsson <n_larsson yahoo.com>
PURPOSE:    CMake module to build GRASS Addons
COPYRIGHT:  (C) 2025 by the GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
]]

#[=======================================================================[.rst:

build_addon
-----------

Build GRASS Addon

  build_addon(<modulename>
              [SOURCES <sources>...]
              [DOCFILES <docfiles>...]
              [GRASSLIBS <grass-libraries>...]
              [DEPENDS <dependency-targets>...]
              [ADDONS <sub-addons>...]
              [NO_DOCS])

  Build an Addon named `<modulename>` with the sourcefiles `<sources>`
  and documentation files `<docfiles>`. For compiled sources GRASS
  library dependencies are added to `GRASSLIBS`, e.g. `gis raster datetime`
  (see the file `GRASSConfig.cmake.in` for supported components). Other
  external dependencies can be added to `DEPENDS` in form of a list of
  targets.

  Wrapper addons, which installs a group of sub-addons, includes each by
  adding a list to `ADDONS`. `NO_DOCS` is an optional for, often, wrapper
  addons, which do not have separate documentation.

  Internally used arguments during recursive calls, listed here for reference:
    [SRC_DIR <source-dir>]
    [PYTHONPATH <python-path>]
    [IS_SCRIPT]


  add_files_to_etc_dir(<dir> FILES <files>...)

  Add `files` destined to the GRASS Addon's `etc/<dir>` directory.
#]=======================================================================]

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if(WIN32)
  set(null_device nul)
  set(search_command findstr /v)
  set(html_search_str "\"</body>\|</html>\|</div> <!-- end container -->\"")
  set(sep "\;")
  string(REPLACE ";" "${sep}" env_path "${env_path}")
else()
  set(null_device /dev/null)
  set(search_command grep -v)
  set(html_search_str "\'</body>\|</html>\|</div> <!-- end container -->\'")
  set(sep ":")
endif()

set(GRASSAddon_BinDIR bin)
set(GRASSAddon_DocDIR docs/html)
set(GRASSAddon_ETCDIR etc)
set(GRASSAddon_LibDIR lib)
set(GRASSAddon_ScriptDIR scripts)

set(OUTDIR "${CMAKE_BINARY_DIR}/output")
file(MAKE_DIRECTORY "${OUTDIR}/${GRASSAddon_BinDIR}")
file(MAKE_DIRECTORY "${OUTDIR}/${GRASSAddon_DocDIR}")
file(MAKE_DIRECTORY "${OUTDIR}/${GRASSAddon_ETCDIR}")
file(MAKE_DIRECTORY "${OUTDIR}/${GRASSAddon_LibDIR}")
file(MAKE_DIRECTORY "${OUTDIR}/${GRASSAddon_ScriptDIR}")

function(add_files_to_etc_dir dir)
  cmake_parse_arguments(PARSE_ARGV 1 G "" "" "FILES")
  file(MAKE_DIRECTORY ${OUTDIR}/${GRASSAddon_ETCDIR}/${dir})
  file(COPY ${G_FILES} DESTINATION ${OUTDIR}/${GRASSAddon_ETCDIR}/${dir})
  install(FILES ${G_FILES} DESTINATION ${GRASSAddon_ETCDIR}/${dir})
endfunction()

function(build_addon name)
  cmake_parse_arguments(PARSE_ARGV 1 G "NO_DOCS;IS_SCRIPT" "SRC_DIR;PYTHONPATH"
                        "SOURCES;ADDONS;DEPENDS;DOCFILES;GRASSLIBS")
  set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR})
  set(G_NAME ${name})
  find_package(GRASS REQUIRED)

  set(MKHTML_PY ${GRASS_UTILS_DIR}/mkhtml.py)
  if(NOT EXISTS ${MKHTML_PY})
    message(FATAL_ERROR "NO MKHTML_PY FOUND")
  endif()

  if(G_ADDONS)
    set(_pythonpath
        "PYTHONPATH=${OUTDIR}/${GRASSAddon_ETCDIR}/${G_NAME}:$ENV{PYTHONPATH}")

    if(NOT G_NO_DOCS)
      set(_tmp_html_file ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.tmp.html)
      set(_out_html_file ${OUTDIR}/${GRASSAddon_DocDIR}/${G_NAME}.html)
      add_custom_command(
        OUTPUT ${_out_html_file}
        COMMAND
          ${_pythonpath} ${CMAKE_COMMAND} -E copy ${_src_dir}/${G_NAME}.html
          ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.html
        COMMAND ${_pythonpath} MODULE_TOPDIR=$ENV{GISBASE} ${PYTHON_EXECUTABLE}
                ${MKHTML_PY} ${G_NAME} > ${_out_html_file}
        COMMAND ${CMAKE_COMMAND} -E remove ${_tmp_html_file}
                ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.html
        COMMENT
          "Creating ${OUT_HTML_FILE} ${OUTDIR}/${GRASSAddon_ETCDIR}/${G_NAME} ${CMAKE_COMMAND}"
      )

      add_custom_target(${G_NAME}-docs ALL DEPENDS ${_out_html_file})

      install(FILES ${_out_html_file} DESTINATION ${GRASSAddon_DocDIR})
    endif()
    foreach(addon IN ITEMS ${G_ADDONS})
      add_subdirectory(${addon})
    endforeach()
    return()
  endif()

  set(_py_files ${G_SOURCES})
  list(FILTER _py_files INCLUDE REGEX ".*\.py$")
  list(LENGTH _py_files _n_py_files)
  if(_n_py_files)
    set(_is_script ON)
  else()
    set(_is_script OFF)
  endif()
  unset(_py_files)
  unset(_n_py_files)

  if(_is_script)
    set(install_dest ${GRASSAddon_ScriptDIR})
    set(PYTHON_EXECUTABLE $ENV{PYTHON_EXECUTABLE})
    _build_addon(
      NAME
      ${G_NAME}
      ${ARGN}
      SRC_DIR
      ${_src_dir}
      IS_SCRIPT
      PYTHONPATH
      ${_pythonpath})
  else()
    _set_thirdparty_include_paths()
    set(install_dest "${GRASSAddon_BinDIR}")
    _build_addon(NAME ${G_NAME} ${ARGN} SRC_DIR ${_src_dir})
  endif()
endfunction()

function(_build_addon)
  cmake_parse_arguments(G "NO_DOCS;IS_SCRIPT" "NAME;SRC_DIR;PYTHONPATH"
                        "SOURCES;DEPENDS;OPTIONAL_DEPENDS;GRASSLIBS;DOCFILES" ${ARGN})

  find_package(GRASS REQUIRED)

  if(G_IS_SCRIPT)
    set(SRC_SCRIPT_FILE ${G_NAME}.py)
    set(_execute ${PYTHON_EXECUTABLE})
    list(FIND G_SOURCES "${SRC_SCRIPT_FILE}" has_script_file)
    if(has_script_file LESS "0" OR NOT EXISTS ${G_SRC_DIR}/${SRC_SCRIPT_FILE})
      message(FATAL_ERROR "${SRC_SCRIPT_FILE} does not exists")
    endif()

    list(TRANSFORM G_SOURCES PREPEND "${G_SRC_DIR}/")

    set(SCRIPT_EXT "")
    if(WIN32)
      set(SCRIPT_EXT ".py")
    endif()

    configure_file(
      ${G_SRC_DIR}/${G_NAME}.py
      ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}${SCRIPT_EXT} COPYONLY)
    file(
      COPY ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME}${SCRIPT_EXT}
      DESTINATION ${OUTDIR}/${GRASSAddon_ScriptDIR}
      FILE_PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE)

    add_custom_target(${G_NAME} ALL)
    target_sources(${G_NAME} PRIVATE ${G_SRC_DIR}/${G_NAME}.py)

    if(WIN32)
      install(PROGRAMS ${OUTDIR}/${GRASSAddon_ScriptDIR}/${G_NAME}.bat
              DESTINATION ${GRASSAddon_ScriptDIR})
    endif()

    install(PROGRAMS ${OUTDIR}/${GRASSAddon_ScriptDIR}/${G_NAME}${SCRIPT_EXT}
            DESTINATION ${GRASSAddon_ScriptDIR})

  else()
    find_package(GRASS REQUIRED COMPONENTS ${G_GRASSLIBS})

    set(CMAKE_INSTALL_RPATH $ENV{GISBASE}/lib)

    list(TRANSFORM G_GRASSLIBS PREPEND "GRASS::")

    set(G_RUNTIME_OUTPUT_DIR "${OUTDIR}/${GRASSAddon_BinDIR}")
    if(MSVC)
      set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${G_RUNTIME_OUTPUT_DIR}>)
    elseif(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
      set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${G_RUNTIME_OUTPUT_DIR})
    endif()

    add_compile_definitions(PACKAGE="grassmods")

    add_executable(${G_NAME} ${G_SOURCES})
    add_dependencies(${G_NAME} ${G_GRASSLIBS} ${G_DEPENDS})

    foreach(G_DEPEND ${G_DEPENDS})
      if(NOT TARGET ${G_DEPEND})
        message(FATAL_ERROR "${G_DEPEND} not a target")
      endif()
      add_dependencies(${G_NAME} ${G_DEPEND})
    endforeach()

    set(opt_depends)
    foreach(optional_dep ${G_OPTIONAL_DEPENDS})
      if(TARGET ${optional_dep})
        add_dependencies(${G_NAME} ${optional_dep})
        list(APPEND opt_depends ${optional_dep})
      else()
        message(WARNING "${optional_dep} not a target")
      endif()
    endforeach()

    target_link_libraries(${G_NAME} ${G_GRASSLIBS} ${G_DEPENDS} ${opt_depends})

    install(TARGETS ${G_NAME} DESTINATION ${install_dest})
  endif()

  if(NOT G_NO_DOCS)
    list(TRANSFORM G_DOCFILES PREPEND "${G_SRC_DIR}/")

    set(html_doc ${G_DOCFILES})
    set(md_doc ${G_DOCFILES})
    set(img_files ${G_DOCFILES})

    list(FILTER html_doc INCLUDE REGEX "${G_NAME}.html$")
    list(FILTER md_doc INCLUDE REGEX "${G_NAME}.md$")
    list(FILTER img_files INCLUDE REGEX ".*\.(jpg|png)$")

    if(img_files)
      set(copy_images_command ${CMAKE_COMMAND} -E copy ${img_files}
                              ${OUTDIR}/${GRASSAddon_DocDIR})
      install(FILES ${img_files} DESTINATION ${GRASSAddon_DocDIR})
    endif()

    set(_tmp_html_file ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.tmp.html)
    set(_out_html_file ${OUTDIR}/${GRASSAddon_DocDIR}/${G_NAME}.html)

    if(EXISTS ${html_doc})
      install(FILES ${_out_html_file} DESTINATION ${GRASSAddon_DocDIR})
    else()
      message(
        FATAL_ERROR
          "${html_doc} does not exists. ${G_SRC_DIR} \n ${OUTDIR}/${GRASSAddon_DocDIR}| ${G_NAME}"
      )
    endif()

    add_custom_command(
      OUTPUT ${_out_html_file}
      COMMAND ${CMAKE_COMMAND} -E copy ${G_SRC_DIR}/${G_NAME}.html
              ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.html
      COMMAND
        ${G_PYTHONPATH} GISBASE=$ENV{GISBASE} GISRC=$ENV{GISRC}
        VERSION_NUMBER=${GRASS_VERSION_STRING}
        VERSION_DATE=${GRASS_VERSION_DATE}
        ${_execute} ${OUTDIR}/${install_dest}/${G_NAME}${SCRIPT_EXT}
        --html-description < ${null_device} | ${search_command}
        ${html_search_str} > ${_tmp_html_file}
      COMMAND MODULE_TOPDIR=$ENV{GISBASE} SOURCE_URL=${SOURCE_URL}
              VERSION_NUMBER=${GRASS_VERSION_STRING} ${PYTHON_EXECUTABLE}
              ${MKHTML_PY} ${G_NAME} > ${_out_html_file}
      COMMAND ${copy_images_command}
      COMMAND ${CMAKE_COMMAND} -E remove ${_tmp_html_file}
              ${CMAKE_CURRENT_BINARY_DIR}/${G_NAME}.html
      COMMENT "Creating ${OUT_HTML_FILE}")

    add_custom_target(${G_NAME}-docs ALL DEPENDS ${G_NAME} ${_out_html_file})
    target_sources(${G_NAME}-docs PRIVATE ${html_doc} ${md_doc} ${img_files})

    install(FILES ${_out_html_file} DESTINATION ${GRASSAddon_DocDIR})
  endif()
endfunction()

# Set Third-party API include paths exposed by GRASS API
macro(_set_thirdparty_include_paths)
  if(NOT TARGET GDAL::GDAL)
    find_package(GDAL)
    if(NOT GDAL_FOUND)
      add_library(GDAL::GDAL INTERFACE)
      set_target_properties(GDAL::GDAL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${GRASS_GDAL_INCLUDE_DIR})
    endif()
  endif()
  if(NOT TARGET PROJ::proj)
    find_package(PROJ QUIET)
    if(NOT PROJ_FOUND)
      add_library(PROJ::proj INTERFACE)
      set_target_properties(PROJ::proj PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES  ${GRASS_PROJ_INCLUDE_DIRS})
    endif()
  endif()
  if(NOT TARGET ZLIB::ZLIB)
    find_package(ZLIB)
    if(NOT ZLIB_FOUND)
      add_library(ZLIB::ZLIB INTERFACE)
      set_target_properties(ZLIB::ZLIB PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES  ${GRASS_ZLIB_INCLUDE_DIR})
    endif()
  endif()
  if(GRASS_WITH_POSTGRES)
    if(NOT TARGET PostgreSQL::PostgreSQL)
      if(NOT PostgreSQL_ADDITIONAL_VERSIONS)
        set(PostgreSQL_ADDITIONAL_VERSIONS "18" "17" "16" "15" "14" "13")
      endif()
      find_package(PostgreSQL)
      if(NOT PostgreSQL_FOUND)
        add_library(PostgreSQL::PostgreSQL INTERFACE)
        set_target_properties(PostgreSQL::PostgreSQL PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES ${GRASS_PostgreSQL_INCLUDE_DIR})
      endif()
    endif()
  endif()
endmacro()

macro(find_OpenMP)
  if(MSVC AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
    set(OpenMP_RUNTIME_MSVC "llvm")
  endif()
  find_package(OpenMP)
  if(OpenMP_FOUND AND MSVC AND CMAKE_VERSION VERSION_LESS "3.30")
    add_compile_options(-openmp:llvm)
  endif()
endmacro()

macro(find_M)
  if(UNIX AND NOT TARGET LIBM)
    find_library(MATH_LIBRARY m)
    add_library(LIBM INTERFACE IMPORTED GLOBAL)
    if(MATH_LIBRARY)
      set_property(TARGET LIBM PROPERTY INTERFACE_LINK_LIBRARIES ${MATH_LIBRARY})
    else()
      # Fallback: allow the linker to resolve -lm by name.
      set_property(TARGET LIBM PROPERTY INTERFACE_LINK_LIBRARIES m)
    endif()
  endif()
endmacro()
