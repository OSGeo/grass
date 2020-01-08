include(GenerateExportHeader)
function(build_module)
  cmake_parse_arguments(G
    "EXE"
    "NAME;SRCDIR;SRC_REGEX;RUNTIME_OUTPUT_DIR;PACKAGE;HTML_FILE_NAME"
    "SOURCES;INCLUDES;DEPENDS;OPTIONAL_DEPENDS;DEFS;HEADERS"
    ${ARGN} )

  if(NOT G_NAME)
    message(FATAL_ERROR "G_NAME empty")
  endif()

  ## update_per_group_target(${G_NAME})

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
    set_target_properties (${G_NAME} PROPERTIES FOLDER bin)
    set(default_html_file_name ${G_NAME})

  else()
    add_library(${G_NAME} ${${G_NAME}_SRCS})
    set_target_properties (${G_NAME} PROPERTIES FOLDER lib)
    set_target_properties(${G_NAME} PROPERTIES OUTPUT_NAME ${G_NAME}.${GRASS_VERSION_NUMBER})
    set(export_file_name "${CMAKE_BINARY_DIR}/include/export/${G_NAME}_export.h")
    # Default is to use library target name without grass_ prefix
    string(REPLACE "grass_" "" default_html_file_name ${G_NAME})

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
 
 set_target_properties(${G_NAME} PROPERTIES RUN_HTML_DESCR "${RUN_HTML_DESCR}")
 set_target_properties(${G_NAME} PROPERTIES G_PGM "$<TARGET_FILE:${G_NAME}>")
 set_target_properties(${G_NAME} PROPERTIES G_SRC_DIR "${G_SRCDIR}")
 set_target_properties(${G_NAME} PROPERTIES G_RUNTIME_OUTPUT_DIR "${G_RUNTIME_OUTPUT_DIR}") 
 set_target_properties(${G_NAME} PROPERTIES G_HTML_FILE_NAME "${HTML_FILE_NAME}.html")
 set_target_properties(${G_TARGET_NAME} PROPERTIES IS_PYTHON_SCRIPT FALSE)
 
 if(WITH_DOCS)
	build_docs(${G_NAME})
 endif() # WITH_DOCS

 install(TARGETS ${G_NAME} DESTINATION ${install_dest})

endfunction()



