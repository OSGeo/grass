include(GenerateExportHeader)
function(build_module)
  cmake_parse_arguments(G  "EXE;NO_HTML_DESCRIPTION" "NAME;SRCDIR;SRC_REGEX" "SOURCES;INCLUDES;DEPENDS;OPTIONAL_DEPENDS;DEFS;HEADERS" ${ARGN} )

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
 #TODO glob for *.html
 file(GLOB html_files "${G_SRCDIR}/*.html")
 set(html_file "${G_SRCDIR}/${G_NAME}.html")

  file(GLOB img_files ${G_NAME}/*.png  ${G_NAME}/*.jpg)
   if(img_files)
   set(img_cmd ${CMAKE_COMMAND} -E copy ${img_files} ${CMAKE_BINARY_DIR}/docs/html)
   else()
   set(img_cmd ${CMAKE_COMMAND} -E echo "")
   endif()

# if(EXISTS "${html_files}")
 if(EXISTS "${html_file}")
	if(${G_NO_HTML_DESCRIPTION})
	set(tmp_html_cmd ${CMAKE_COMMAND} -E echo "")
	else()
	set(tmp_html_cmd ${CMAKE_BINARY_DIR}/tools/run_grass.bat ${G_NAME} --html-description)
	endif()
	set(mkhtml_cmd ${CMAKE_BINARY_DIR}/tools/run_python.bat ${CMAKE_BINARY_DIR}/tools/mkhtml.py)

	set(html_file_tmp "${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html")
	set(html_file_out "${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.html")

	  ADD_CUSTOM_COMMAND(TARGET ${G_NAME} POST_BUILD
	  COMMAND ${tmp_html_cmd} > ${html_file_tmp}
	  COMMAND ${mkhtml_cmd} ${G_NAME} ${html_file} ${html_file_tmp} > ${html_file_out}
	  COMMAND ${CMAKE_COMMAND} -E remove ${html_file_tmp}
	  COMMAND ${img_cmd}
	  )



	  #set_source_files_properties(${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.html PROPERTIES GENERATED TRUE)
    #add_custom_target(${G_NAME}_doc ALL
     # COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/docs/html/${G_NAME}.tmp.html
     # 
	 # set_target_properties (${G_NAME}_doc PROPERTIES FOLDER docs)
   endif()
endfunction()
