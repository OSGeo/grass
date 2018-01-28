macro(make_script_in_dir dir_name)
  build_grass_script(NAME ${dir_name})
endmacro()

function(build_grass_script)
    cmake_parse_arguments(G  "" "NAME;SRCDIR;SRC_REGEX" "SOURCES;DEPENDS" ${ARGN} )

    update_per_group_target( ${G_NAME} )
    if(NOT G_SRCDIR)
      set(SCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${G_NAME})
    endif()


    if(NOT G_SRC_REGEX)
      set(G_SRC_REGEX "*.py")
    endif()

    file(GLOB SRC_FILES "${SCRIPT_DIR}/${G_SRC_REGEX}")
    if(NOT SRC_FILES)
      message(FATAL_ERROR "[ ${G_NAME} ]: SRC_FILES empty ")
    endif()


#  message(FATAL_ERROR "ETCPYFILES=${ETCPYFILES}")
#install(FILES ${ETCPYCFILES} DESTINATION etc)

if(MINGW)
  #PGM script_name
  # $(BIN)/$(PGM).bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
  # sed -e "s#SCRIPT_NAME#$(PGM)#" -e "s#SCRIPT_DIR#$(SCRIPT_DIR)#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@
  # unix2dos $@
  configure_file(
    ${CMAKE_SOURCE_DIR}/scripts/windows_launch.bat
    ${CMAKE_BINARY_DIR}/bin/${G_NAME}.bat
    )
  install( PROGRAMS ${CMAKE_BINARY_DIR}/bin/${G_NAME}.bat DESTINATION bin )
endif()

foreach(pyfile ${SRC_FILES})
  get_filename_component(pyfile_NAME ${pyfile} NAME)

  string(REPLACE ".py" "" pyfile_NAME ${pyfile_NAME})
  
  set(${pyfile_NAME}_OUTPUT_FILE ${CMAKE_BINARY_DIR}/locale/scriptstrings/${pyfile_NAME}_to_translate.c)

  add_custom_command(
    OUTPUT ${${pyfile_NAME}_OUTPUT_FILE}.stamp
    DEPENDS g.parser
    COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${pyfile}
      -DOUTPUT_FILE=${${pyfile_NAME}_OUTPUT_FILE}
      -DBIN_DIR=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
    COMMAND ${CMAKE_COMMAND} -E touch ${${pyfile_NAME}_OUTPUT_FILE}.stamp
    COMMENT "Generating ${${pyfile_NAME}_OUTPUT_FILE}"
    VERBATIM
    )
  
  add_custom_target(${pyfile_NAME}
    ALL
    DEPENDS ${${pyfile_NAME}_OUTPUT_FILE}.stamp
    )

  # add_custom_target(${pyfile_NAME} "${${pyfile_NAME}_OUTPUT_FILE}"
  #   COMMAND ${CMAKE_COMMAND}
  #   -DINPUT_FILE=${pyfile}
  #   -DOUTPUT_FILE=${${pyfile_NAME}_OUTPUT_FILE}
  #   -DBIN_DIR=${CMAKE_BINARY_DIR}
  #   -P ${CMAKE_SOURCE_DIR}/cmake/locale_strings.cmake
  #   DEPENDS g.parser
  #   COMMENT "Generating ${${pyfile_NAME}_OUTPUT_FILE}"
  #   )

#  set_source_files_properties("${${pyfile_NAME}_OUTPUT_FILE}" GENERATED)
  
  foreach(G_DEPEND ${G_DEPENDS})
    add_dependencies(${G_DEPEND})
  endforeach()
  
  install(FILES ${pyfile}
    RENAME ${pyfile_NAME}
    DESTINATION scripts)
endforeach()

endfunction()



