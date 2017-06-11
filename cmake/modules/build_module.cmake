
# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE: 	 This is the main function that builds all grass libraries (prefixed with grass_)
#            and grass exeuctables. This cmake function is tailored to meet requirement of grass gnu make rules
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.

include(GenerateExportHeader)
function(build_module)
  cmake_parse_arguments(G
    "EXE"
    "NAME;SRCDIR;SRC_REGEX;RUNTIME_OUTPUT_DIR;PACKAGE;HTML_FILE_NAME"
    "SOURCES;INCLUDES;DEPENDS;OPTIONAL_DEPENDS;PRIMARY_DEPENDS;DEFS;HEADERS;TEST_SOURCES"
    ${ARGN} )

  if(NOT G_NAME)
    message(FATAL_ERROR "G_NAME empty")
  endif()

  foreach(PRIMARY_DEPEND ${G_PRIMARY_DEPENDS})
	if (NOT TARGET ${PRIMARY_DEPEND})
		message(STATUS "${G_NAME} disabled because ${PRIMARY_DEPEND} is not available")
		return()
	else()
	list(APPEND G_DEPENDS ${PRIMARY_DEPEND})
	endif()
  endforeach()

  if(NOT G_SRC_REGEX)
    set(G_SRC_REGEX "*.c")
  endif()

  if(NOT G_SRCDIR)
    set(G_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
   set(html_file "${G_SRCDIR}/${G_NAME}.html")

  foreach(G_HEADER ${G_HEADERS})
    if( EXISTS "${G_SRCDIR}/${G_HEADER}" )
      file(COPY ${G_SRCDIR}/${G_HEADER} DESTINATION "${CMAKE_BINARY_DIR}/include/grass")
    else()
      file(GLOB header_list_from_glob LIST_DIRECTORIES false "${G_SRCDIR}/${G_HEADER}")
      if(NOT header_list_from_glob)
	message(FATAL_ERROR "MUST copy '${G_SRCDIR}/${G_HEADER}' to ${CMAKE_BINARY_DIR}/include/grass")
      endif()
      foreach(header_I ${header_list_from_glob})
	file(COPY ${header_I} DESTINATION "${CMAKE_BINARY_DIR}/include/grass")
      endforeach()
    endif()
  endforeach()

  if(NOT G_SOURCES)
    file(GLOB ${G_NAME}_SRCS "${G_SRCDIR}/${G_SRC_REGEX}")
  else()
    set(${G_NAME}_SRCS ${G_SOURCES})
  endif()

  if(G_EXE)
    add_executable(${G_NAME} ${${G_NAME}_SRCS})
	if("${G_NAME}" MATCHES "^v.*")
	set_target_properties (${G_NAME} PROPERTIES FOLDER vector)
	elseif("${G_NAME}" MATCHES "^r.*")
	set_target_properties (${G_NAME} PROPERTIES FOLDER raster)
	else()
    set_target_properties (${G_NAME} PROPERTIES FOLDER bin)
	endif()
    set(default_html_file_name ${G_NAME})
	set(PGM_NAME ${G_NAME})
	if(WIN32)
	set(PGM_NAME ${G_NAME}.exe)
	endif()

	set(modules_list "${G_NAME};${modules_list}" CACHE INTERNAL "list of modules")


  else()
    add_library(${G_NAME} ${${G_NAME}_SRCS})
    set_target_properties (${G_NAME} PROPERTIES FOLDER lib)
    set_target_properties(${G_NAME} PROPERTIES OUTPUT_NAME ${G_NAME}.${GRASS_VERSION_NUMBER})
    set(export_file_name "${CMAKE_BINARY_DIR}/include/export/${G_NAME}_export.h")
    # Default is to use library target name without grass_ prefix
    string(REPLACE "grass_" "" default_html_file_name ${G_NAME})
	set(PGM_NAME ${default_html_file_name})

    generate_export_header(${G_NAME}
      STATIC_DEFINE "STATIC_BUILD"
      EXPORT_FILE_NAME ${export_file_name})
  endif()

  if(G_HTML_FILE_NAME)
    set(HTML_FILE_NAME ${G_HTML_FILE_NAME})
  else()
    set(HTML_FILE_NAME ${default_html_file_name})
  endif()

  get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
  set_property(GLOBAL PROPERTY MODULE_LIST "${MODULE_LIST};${G_NAME}")

  add_dependencies(${G_NAME} copy_header)

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
   list(APPEND ${G_NAME}_INCLUDE_DIRS "${G_SRCDIR}")
   foreach(G_INCLUDE ${G_INCLUDES})
     list(APPEND ${G_NAME}_INCLUDE_DIRS "${G_INCLUDE}")
   endforeach()

   if(${G_NAME}_INCLUDE_DIRS)
     list(REMOVE_DUPLICATES ${G_NAME}_INCLUDE_DIRS )
   endif()

   target_include_directories(${G_NAME} PUBLIC ${${G_NAME}_INCLUDE_DIRS})
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

 target_compile_definitions(${G_NAME} PRIVATE "-DPACKAGE=\"${package_define}\"")

 foreach(dep ${G_DEPENDS} ${G_OPTIONAL_DEPENDS})
   if(TARGET ${dep})
     get_target_property(interface_def ${dep} INTERFACE_COMPILE_DEFINITIONS)
     if(interface_def)
       target_compile_definitions(${G_NAME} PRIVATE "${interface_def}")
     endif()
   endif()
   target_link_libraries(${G_NAME} ${dep})
 endforeach()

 set(RUN_HTML_DESCR TRUE)
 # Auto set if to run RUN_HTML_DESCR
 if(G_EXE)
   set(RUN_HTML_DESCR TRUE)
   if (G_RUNTIME_OUTPUT_DIR)
     set(RUN_HTML_DESCR FALSE)
   endif()
   # g.parser and some others does not have --html-description.
   if( ${G_NAME} IN_LIST NO_HTML_DESCR_TARGETS)
     set(RUN_HTML_DESCR FALSE)
   endif()
 else()
   set(RUN_HTML_DESCR FALSE)
 endif()

 set(install_dest "")
 if(NOT G_RUNTIME_OUTPUT_DIR)
   if(G_EXE)
     set(G_RUNTIME_OUTPUT_DIR "${GISBASE}/bin")
     set(install_dest "bin")
   else()
     set(G_RUNTIME_OUTPUT_DIR "${GISBASE}/lib")
     set(install_dest "lib")
   endif()
 else()
   set(install_dest "${G_RUNTIME_OUTPUT_DIR}")
   set(G_RUNTIME_OUTPUT_DIR "${GISBASE}/${install_dest}")
 endif()

 # To use this property later in build_docs
 	set(PGM_EXT "")
	if(WIN32)
		if(G_EXE)
			set(PGM_EXT ".exe")
		endif()
	endif()


 if(WITH_DOCS)

  set(G_HTML_FILE_NAME "${HTML_FILE_NAME}.html")

  set(html_file ${G_SRCDIR}/${G_HTML_FILE_NAME})
  set(HTML_FILE)
  set(no_docs_list "grass_sqlp;echo;clean_temp;lock;run")

  if(EXISTS ${html_file})
    set(HTML_FILE ${html_file})
	install(FILES ${GISBASE}/docs/html/${G_HTML_FILE_NAME} DESTINATION docs/html)
  else()
    file(GLOB html_files ${G_SRCDIR}/*.html)
    if(html_files)
      if(NOT ${target_name} IN_LIST no_docs_list)
		message(FATAL_ERROR "${html_file} does not exists. ${G_SRCDIR} \n ${G_RUNTIME_OUTPUT_DIR} | ${target_name}")
      endif()
    endif()
  endif()


  if(NOT HTML_FILE)
	add_custom_command(TARGET ${G_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${G_NAME}> ${G_RUNTIME_OUTPUT_DIR}
	)
	return()
  endif()
##message("HTML_FILE=${HTML_FILE}")

get_filename_component(HTML_FILE_NAME ${HTML_FILE} NAME)
get_filename_component(PGM_SOURCE_DIR ${HTML_FILE} PATH)

string(REPLACE ".html"  "" PGM_NAME "${HTML_FILE_NAME}" )
string(REPLACE ".html" ".tmp.html" TMP_HTML_NAME ${HTML_FILE_NAME})
set(TMP_HTML_FILE ${G_SRCDIR}/${TMP_HTML_NAME})
set(OUT_HTML_FILE ${GISBASE}/docs/html/${HTML_FILE_NAME})

set(PGM_EXT "")
if(WIN32)
  set(PGM_EXT ".exe")
endif()

set(html_descr_argument "--html-description")
if(RUN_HTML_DESCR)
set(html_descr_command ${G_NAME}${PGM_EXT} "--html-description")
else()
  set(html_descr_command ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME})
endif()

file(GLOB IMG_FILES ${G_SRCDIR}/*.png  ${G_SRCDIR}/*.jpg)
set(copy_images_command ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${G_NAME})
if(IMG_FILES)
	set(copy_images_command ${CMAKE_COMMAND} -E copy ${IMG_FILES} ${GISBASE}/docs/html/)
endif()

add_custom_command(TARGET ${G_NAME} POST_BUILD
  COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRCDIR}
  ${html_descr_command} > ${TMP_HTML_FILE}
  COMMAND ${grass_env_command} ${CMAKE_COMMAND} -E chdir ${G_SRCDIR}
  ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${PGM_NAME} > ${OUT_HTML_FILE}
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${G_NAME}> ${G_RUNTIME_OUTPUT_DIR}
  COMMAND ${copy_images_command}
  COMMAND ${CMAKE_COMMAND} -E remove ${TMP_HTML_FILE}
  COMMENT "Creating ${OUT_HTML_FILE}")

endif() # WITH_DOCS

foreach(test_SOURCE ${G_TEST_SOURCES})
	add_test(NAME  ${G_NAME}-test
         COMMAND ${grass_env_command}  ${PYTHON_EXECUTABLE}
		 ${G_SRCDIR}/testsuite/${test_SOURCE})
	message("[build_module] ADDING TEST ${G_NAME}-test")
endforeach()

 install(TARGETS ${G_NAME} DESTINATION ${install_dest})

endfunction()
