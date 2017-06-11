cmake_minimum_required(VERSION 3.3)

project(GRASSGIS)

set(BUILD_SHARED_LIBS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/find_scripts;${CMAKE_MODULE_PATH}")

#TODO
option(WITH_CAIRO "Build with cairo support ." ON)
option(WITH_X11 "Build with X11 support ." ON)
option(WITH_OPENGL "Build with opengl support ." OFF)

set(GRASS_VERSION_MAJOR)
set(GRASS_VERSION_MINOR)
set(GRASS_VERSION_PATCH)


#TODO
set(GRASS_VERSION_UPDATE_PKG "0.2")
set(BUILD_ARCH "x86_64")

file(STRINGS "include/VERSION" GRASS_VERSION_STRINGS)
list(LENGTH GRASS_VERSION_STRINGS GRASS_VERSION_FILE_LENGTH)

if(GRASS_VERSION_FILE_LENGTH LESS 3 )
  message(FATAL_ERROR "include/VERSION is not a valid file")
endif()

list(GET GRASS_VERSION_STRINGS 0 GRASS_VERSION_MAJOR)
list(GET GRASS_VERSION_STRINGS 1 GRASS_VERSION_MINOR)

if(GRASS_VERSION_FILE_LENGTH GREATER 3 )
  list(GET GRASS_VERSION_STRINGS 2 GRASS_VERSION_RELEASE)
endif()

if(GRASS_VERSION_FILE_LENGTH GREATER 4 )
  list(GET GRASS_VERSION_STRINGS 3 GRASS_VERSION_DATE)
endif()

set(GRASS_VERSION_NUMBER ${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.${GRASS_VERSION_RELEASE})

set(NAME_VER ${GRASS_VERSION_MAJOR})
set(LIB_VER "${GRASS_VERSION_MAJOR}.${GRASS_VERSION_MINOR}.")

set(GRASS_VERSION_SVN "exported")
find_program(SVN_VERSION NAMES svnversion)
if(SVN_VERSION)
  execute_process(
    COMMAND ${SVN_VERSION} -c
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../addons/grass7
    OUTPUT_VARIABLE svnversion_OV
    ERROR_VARIABLE  svnversion_EV
    RESULT_VARIABLE svnversion_RV
    )

  if(svnversion_RV )
    message(FATAL_ERROR "Error running svnversion=${svnversion_EV}")
  else()
    string(REGEX REPLACE "([0-9]+):" "r" GRASS_VERSION_SVN "${svnversion_OV}")
    string(STRIP ${GRASS_VERSION_SVN} GRASS_VERSION_SVN)
  endif()
  #message(FATAL_ERROR "GRASS_VERSION_SVN=${GRASS_VERSION_SVN}| ${svnversion_OV}")
    
endif() #SVN_VERSION

message(STATUS "GRASS_VERSION_NUMBER  =  '${GRASS_VERSION_NUMBER}'")
message(STATUS "GRASS_VERSION_SVN     =  '${GRASS_VERSION_SVN}'")

# Setup build locations.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

function(update_per_group_target t_name)
  if(${t_name} MATCHES "(^([a-z|0-9]+)\\.)")
    string(REPLACE "." ";" name_to_list "${t_name}")
    list(GET name_to_list 0 all_target_name)
    if(all_target_name)
      set(all_target_name ${all_target_name}-all)
    else()
      message(FATAL_ERROR "all_target_name=${all_target_name} \t t_name=${t_name}")      
    endif()
    
    if(TARGET ${all_target_name})
      add_dependencies(${all_target_name} ${t_name})
    else()
      add_custom_target(${all_target_name} DEPENDS ${t_name})
    endif()
  endif()

endfunction()

function(build_module)
  cmake_parse_arguments(G  "EXE" "NAME;SRCDIR;SRC_REGEX" "SOURCES;INCLUDES;DEPENDS;DEFS;HEADERS" ${ARGN} )

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
  else()
    add_library(${G_NAME} ${${G_NAME}_SRCS})
    set_target_properties(${G_NAME}
      PROPERTIES OUTPUT_NAME ${G_NAME}.${GRASS_VERSION_NUMBER}
      )
  endif()

  set(${G_NAME}_INCLUDE_DIRS)

  list(APPEND ${G_NAME}_INCLUDE_DIRS "${G_SRCDIR}")
  
  if(G_INCLUDES)
    foreach(G_INCLUDE ${G_INCLUDES})
 #     message( "xx=${G_INCLUDE} ${G_NAME}" )
      target_include_directories(${G_NAME} PUBLIC ${G_INCLUDE} )
    endforeach()
  endif()
  
 set(${G_NAME}_LINK_LIBS)  

  foreach(G_DEPEND ${G_DEPENDS})
    if(NOT TARGET ${G_DEPEND})
      message(FATAL_ERROR "${G_DEPEND} not a target")
      break()
    endif()

    #message( "${G_NAME} depends on \t ${G_DEPEND}")
    ##go recurse on depends()
    add_dependencies(${G_NAME} ${G_DEPEND})


    


    get_target_property(${G_DEPEND}_IDIRS ${G_DEPEND} INTERFACE_INCLUDE_DIRECTORIES)

    foreach(${G_DEPEND}_IDIR ${${G_DEPEND}_IDIRS})
	if(EXISTS "${${G_DEPEND}_IDIR}")
	  list(APPEND ${G_NAME}_INCLUDE_DIRS "${${G_DEPEND}_IDIR}")
	else()
	  #message(FATAL_ERROR "${G_DEPEND} has incorrect value for INTERFACE_INCLUDE_DIRECTORIES property [${${G_DEPEND}_IDIRS}]")
	endif()

      endforeach()
      

      get_target_property(${G_DEPEND}_IMPORTED_LOCATIONS ${G_DEPEND} IMPORTED_LOCATION)
      foreach( ${G_DEPEND}_IMPORTED_LOC ${${G_DEPEND}_IMPORTED_LOCATIONS})
	if(NOT EXISTS "${${G_DEPEND}_IMPORTED_LOC}")
	  list(APPEND ${G_NAME}_LINK_LIBS "${G_DEPEND}")
	else()
	  list(APPEND ${G_NAME}_LINK_LIBS "${${G_DEPEND}_IMPORTED_LOC}")
	endif()
      endforeach()

    
    #message("$<TARGET_PROPERTY:LIB_GDAL,INTERFACE_INCLUDE_DIRECTORIES> ")
    #message("$<TARGET_PROPERTY:LIB_GDAL,IMPORTED_LOCATION> ")

  endforeach()

#  if(G_EXE)
#    target_link_libraries(${G_NAME} m)
#  endif()

if(G_DEFS)
    foreach(G_DEF ${G_DEFS})
      target_compile_definitions(${G_NAME} PUBLIC "${G_DEF}")
    endforeach()
  endif()

  if(${G_NAME}_LINK_LIBS)
    list(REMOVE_DUPLICATES ${G_NAME}_LINK_LIBS )
    #   message("${G_NAME}_LINK_LIBS=${${G_NAME}_LINK_LIBS}")
    target_link_libraries(${G_NAME} ${${G_NAME}_LINK_LIBS})
endif()

  if(${G_NAME}_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES ${G_NAME}_INCLUDE_DIRS )

#    message("${G_NAME}_INCLUDE_DIRS=${${G_NAME}_INCLUDE_DIRS}")
    
 
endif()

target_include_directories(${G_NAME} PUBLIC ${${G_NAME}_INCLUDE_DIRS})

#foreach( ${G_NAME}_INCLUDE_DIR ${${G_NAME}_INCLUDE_DIRS})
#  target_include_directories(${G_NAME} PUBLIC ${${G_NAME}_INCLUDE_DIR})
#endforeach()

  set_property( TARGET ${G_NAME} PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/lib${G_NAME}.a;${${G_NAME}_LINK_LIBS}" )
  set_property( TARGET ${G_NAME} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${${G_NAME}_INCLUDE_DIRS}" )

endfunction()  


macro(build_library_in_subdir dir_name)
  set (extra_args ${ARGN})
  if ("NAME" IN_LIST extra_args)
    #message("dir_name=${dir_name} ${extra_args}")
    build_module(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN} )
  else()
    get_filename_component(g_name ${dir_name} NAME)
    #message("dir_name=${dir_name} |g_name= ${g_name}")
    build_module(NAME grass_${g_name} SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN})
  endif()
endmacro()

macro(build_program)
  build_module(${ARGN} EXE)
endmacro()

macro(build_program_in_subdir dir_name)
  set (extra_args ${ARGN})
  if ("NAME" IN_LIST extra_args)
    #message("dir_name=${dir_name} ${extra_args}")
    build_program(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN} )
  else()
    get_filename_component(pgm_name ${dir_name} NAME)
    build_program(NAME ${pgm_name} SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir_name} ${ARGN} )
  endif()
endmacro()


add_subdirectory(thirdparty)

include(cmake/scripts.cmake)

add_subdirectory(include)

#include_directories("${CMAKE_SOURCE_DIR}/dist.x86_64-unknown-openbsd6.0/include")

include_directories("${CMAKE_BINARY_DIR}/include")

add_subdirectory(lib)

set(ALL_SUBDIRS
  general
  db
  display
  imagery
  misc
  raster
  raster3d
  demolocation
  scripts
  vector  
  temporal
  tools

  gui


  
  # ps
  # locale
  # man
  # doc
  # testsuite
  # demolocation
  # macosx  
  # mswindows
)



foreach(d ${ALL_SUBDIRS})
  add_subdirectory(${d})
endforeach()




if(WITH_X11)
  build_program_in_subdir(visualization/ximgview DEPENDS grass_gis X11)
endif()
