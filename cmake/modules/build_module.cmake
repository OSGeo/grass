include(GenerateExportHeader)
function(build_module)
  cmake_parse_arguments(G  "EXE" "NAME;SRCDIR;SRC_REGEX;NO_HTML_DESCR" "SOURCES;INCLUDES;DEPENDS;OPTIONAL_DEPENDS;DEFS;HEADERS" ${ARGN} )

  if(NOT G_NAME)
    message(FATAL_ERROR "G_NAME empty")
  endif()

  update_per_group_target( ${G_NAME} )

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
  else()
    add_library(${G_NAME} ${${G_NAME}_SRCS})
	set_target_properties (${G_NAME} PROPERTIES FOLDER lib)
    set_target_properties(${G_NAME} PROPERTIES OUTPUT_NAME ${G_NAME}.${GRASS_VERSION_NUMBER})

	set(export_file_name "${CMAKE_BINARY_DIR}/include/export/${G_NAME}_export.h")
	generate_export_header(${G_NAME}
	  STATIC_DEFINE "STATIC_BUILD"
	  EXPORT_FILE_NAME ${export_file_name})
  endif()

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

 foreach(dep ${G_DEPENDS} ${G_OPTIONAL_DEPENDS})
   if(TARGET ${dep})
     get_target_property(interface_def ${dep} INTERFACE_COMPILE_DEFINITIONS)
	 if(interface_def)
       target_compile_definitions(${G_NAME} PRIVATE "${interface_def}")
	 endif()
   endif()
    target_link_libraries(${G_NAME} ${dep})
 endforeach()

 if(G_EXE)
    install(TARGETS ${G_NAME} DESTINATION bin)
 else()
    install(TARGETS ${G_NAME} DESTINATION lib)
 endif()

 set(html_file "${G_SRCDIR}/${G_NAME}.html")
 if(EXISTS "${html_file}")
   if(NOT ${G_NO_HTML_DESCR})
     add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html
	   COMMAND ${CMAKE_BINARY_DIR}/tools/run_grass.bat
	   ${CMAKE_BINARY_DIR}/bin/${G_NAME}
	   --html-description > ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html
	   DEPENDS ${G_NAME}
	   COMMENT "Generating ${G_NAME}.tmp.html"
	   VERBATIM)
    else()
	  file(WRITE ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html  "")
	endif()

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.html
	  COMMAND ${CMAKE_BINARY_DIR}/tools/run_python.bat
	  ${CMAKE_BINARY_DIR}/tools/mkhtml.py ${CMAKE_BINARY_DIR}/bin/${G_NAME}
	  ${html_file} > ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.html
	  DEPENDS ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html
	  COMMENT "Generating docs/html/${G_NAME}.html"
      VERBATIM)

    add_custom_target(${G_NAME}_doc ALL
      COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html
      DEPENDS ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.html)
	  set_target_properties (${G_NAME}_doc PROPERTIES FOLDER docs)
   endif()
endfunction()
