set(WXPYTHON_DIR ${GRASS_INSTALL_GUIDIR}/wxpython)
set(gui_lib_DIRS
    core
    gui_core
    history
    icons
    iscatt
    lmgr
    location_wizard
    main_window
    mapdisp
    mapwin
    modules
    nviz
    startup
    vnet
    web_services
    wxplot)

set(gui_lib_targets)
foreach(gui_lib_DIR ${gui_lib_DIRS})
  copy_python_files_in_subdir(${gui_lib_DIR} ${WXPYTHON_DIR})
  list(APPEND gui_lib_targets python_${gui_lib_DIR})
  install(DIRECTORY "${OUTDIR}/${WXPYTHON_DIR}/${gui_lib_DIR}"
          DESTINATION "${WXPYTHON_DIR}")
endforeach()
add_custom_target(
  GUI_WXPYTHON
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wxgui.py
          ${OUTDIR}/${WXPYTHON_DIR}
  DEPENDS ${gui_lib_targets} LIB_PYTHON)

set(gui_DIRS
    animation
    datacatalog
    dbmgr
    gcp
    gmodeler
    iclass
    image2target
    mapswipe
    photo2image
    psmap
    rlisetup
    timeline
    tplot
    rdigit
    vdigit)
set(g_gui_targets)
foreach(gui_DIR ${gui_DIRS})
  build_gui_in_subdir(${gui_DIR})
  list(APPEND g_gui_targets "g.gui.${gui_DIR}")
endforeach()

if(WITH_DOCS)
  add_subdirectory(docs)
endif()

install(FILES README.md ${OUTDIR}/${WXPYTHON_DIR}/wxgui.py
        DESTINATION ${WXPYTHON_DIR})

add_custom_target(
  copy_wxpython_xml
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/xml
          ${OUTDIR}/${WXPYTHON_DIR}/xml)

add_dependencies(copy_wxpython_xml GUI_WXPYTHON)

add_custom_target(
  compile_python_files
  COMMAND ${PYTHON_EXECUTABLE} -m compileall
          ${OUTDIR}/${GRASS_INSTALL_SCRIPTDIR}
  COMMAND ${PYTHON_EXECUTABLE} -m compileall ${OUTDIR}/${WXPYTHON_DIR})

add_dependencies(compile_python_files ALL_MODULES)

add_custom_command(
  OUTPUT "${OUTDIR}/${WXPYTHON_DIR}/xml/module_items.xml"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/tools"
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_modules_xml.py tools
  COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} tools/build_modules_xml.py >
          "${OUTDIR}/${WXPYTHON_DIR}/xml/module_items.xml"
  COMMENT "Generating interface description for all modules...")

add_custom_target(build_modules_items_xml
                  DEPENDS "${OUTDIR}/${WXPYTHON_DIR}/xml/module_items.xml")

add_dependencies(build_modules_items_xml copy_wxpython_xml grass_interface_dtd
                 compile_python_files ${g_gui_targets})

add_custom_target(
  build_xml_menudata
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py >
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "validate"
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/menudata.xml
    ${OUTDIR}/${WXPYTHON_DIR}/xml
  DEPENDS build_modules_items_xml)

add_custom_target(
  build_module_tree_menudata
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "module_tree" >
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/toolboxes.py "validate"
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/module_tree_menudata.xml
    ${OUTDIR}/${WXPYTHON_DIR}/xml
  DEPENDS build_xml_menudata)

add_custom_target(
  build_menustrings ALL
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "manager" >>
    ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "module_tree" >>
    ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  COMMAND
    ${grass_env_command} ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/menutree.py "psmap" >>
    ${CMAKE_CURRENT_SOURCE_DIR}/menustrings.py
  DEPENDS build_module_tree_menudata gui_images)

install(DIRECTORY "${OUTDIR}/${WXPYTHON_DIR}/xml" DESTINATION "${WXPYTHON_DIR}")
