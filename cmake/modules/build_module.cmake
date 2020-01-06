include(GenerateExportHeader)
function(build_module)
  cmake_parse_arguments(G  "EXE" "NAME;SRCDIR;SRC_REGEX;RUNTIME_OUTPUT_DIR;PACKAGE" "SOURCES;INCLUDES;DEPENDS;OPTIONAL_DEPENDS;DEFS;HEADERS" ${ARGN} )

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
  #set(MODULE_LIST "${MODULE_LIST} ${G_NAME}")
  #list(APPEND MODULE_LIST ${G_NAME})
  #SET(MODULE_LIST  "${MODULE_LIST} ${G_NAME}" CACHE INTERNAL "source_list")
  get_property(MODULE_LIST GLOBAL PROPERTY MODULE_LIST)
  set_property(GLOBAL PROPERTY MODULE_LIST "${MODULE_LIST} ${G_NAME}")

#message(FATAL_ERROR "EXCLUDED_MODULE_LIST=${EXCLUDED_MODULE_LIST}")
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
   #auto set if to run RUN_HTML_DESCR
   if(G_EXE)
     set(RUN_HTML_DESCR TRUE)
     if (G_RUNTIME_OUTPUT_DIR)
       set(RUN_HTML_DESCR FALSE)
     endif()
   # g.parser does not have --html-description.
   if( ${G_NAME} IN_LIST NO_HTML_DESCR_TARGETS)
     set(RUN_HTML_DESCR FALSE)
   endif()     
   else()
     set(RUN_HTML_DESCR FALSE)
   endif()

#   message(" ${G_NAME} == ${RUN_HTML_DESCR}") 


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
 
  
 if(WITH_DOCS)
 set(html_files)
 set(html_file "${G_SRCDIR}/${G_NAME}.html")
 set(create_html FALSE)
 if(EXISTS "${html_file}")
   set(create_html TRUE)
   file(GLOB img_files ${G_NAME}/*.png  ${G_NAME}/*.jpg)
 else()
   set(html_file "")
   file(GLOB html_files "${G_SRCDIR}/*.html")
    #message("html_files=${html_files}")
   if(html_files)
     # TODO; check if there is more than 1 .html for libs
     list(GET html_files 0 html_file)
     set(create_html TRUE)
   endif()
 endif()


# message("html_file=${html_file}")
# message(FATAL_ERROR "create_html=${create_html}")
 if(create_html)
   get_filename_component(html_name ${html_file} NAME)
   string(REPLACE ".html" "" html_name ${html_name})
   file(GLOB img_files ${G_SRCDIR}/*.png  ${G_SRCDIR}/*.jpg)

   set(html_file_tmp "${GISBASE}/docs/html/${html_name}.tmp.html")
   set(html_file_out "${GISBASE}/docs/html/${html_name}.html")
    add_custom_command(TARGET ${G_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${G_NAME}> ${G_RUNTIME_OUTPUT_DIR})
     if(RUN_HTML_DESCR)
       add_custom_command(TARGET ${G_NAME} POST_BUILD	 
     	 COMMAND ${RUN_GRASS} ${G_NAME} --html-description  > ${html_file_tmp}
     	 COMMENT "Generating ${html_file_tmp}")
     endif()
     
     if(img_files)
    add_custom_command(TARGET ${G_NAME} POST_BUILD	 
     	 COMMAND ${CMAKE_COMMAND} -E copy ${img_files} ${GISBASE}/docs/html/
     	 #COMMENT "Copy ${G_SRCDIR}/{*.png,*.jpg} to ${GISBASE}/docs/html/"
	 )
     endif()
     
     set(mkhtml_cmd ${RUN_PYTHON} ${CMAKE_BINARY_DIR}/tools/mkhtml.py)
    add_custom_command(TARGET ${G_NAME} POST_BUILD
      COMMAND ${mkhtml_cmd} ${html_name} ${html_file} ${html_file_tmp} > ${html_file_out}
      COMMAND ${CMAKE_COMMAND} -E remove ${html_file_tmp}
      COMMENT "Generating ${html_file_out}"
    )
  install(FILES ${html_file_out} DESTINATION docs/html)
#     add_custom_target(${G_NAME}_docs ALL DEPENDS ${html_file_out})
   endif() # if(EXISTS "${html_file}")
   
 endif() #WITH_DOCS


 install(TARGETS ${G_NAME} DESTINATION ${install_dest})

endfunction()
