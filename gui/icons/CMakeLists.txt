file(GLOB SRCICO "*.ico" "*.png")
file(GLOB SRCICONS "grass/*.png")
file(GLOB SRCFLAGS "flags/*.png")
set(icons_stamp_file ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/GUI_ICONS.stamp)
add_custom_command(OUTPUT ${icons_stamp_file}
COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/icons/grass
COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/flags
COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/docs/html/icons
COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/images/symbols
COMMAND ${CMAKE_COMMAND} -E copy ${SRCICO} ${GISBASE}/gui/icons/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/grass ${GISBASE}/gui/icons/
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/flags ${CMAKE_BINARY_DIR}/gui/icons/
COMMAND ${CMAKE_COMMAND} -E touch ${icons_stamp_file})

set(output_icons)
foreach(SRCICON ${SRCICONS})
	get_filename_component(FILE_NAME ${SRCICON} NAME)
	add_custom_command(OUTPUT ${GISBASE}/docs/html/${FILE_NAME}
		COMMAND ${CMAKE_COMMAND} -E copy ${SRCICON} ${GISBASE}/docs/html)
	list(APPEND output_icons ${GISBASE}/docs/html/${FILE_NAME})
endforeach()
add_custom_target(copy_gui_icons DEPENDS ${output_icons} ${icons_stamp_file})

install(FILES ${SRCICO} DESTINATION gui/icons)
install(FILES ${SRCICONS} DESTINATION gui/icons/grass)
install(FILES ${SRCFLAGS} DESTINATION gui/icons/flags)
install(FILES ${SRCICONS} DESTINATION docs/html/icons)

install(FILES grass.desktop DESTINATION share/applications)
foreach(icon_type 8 16 22 24 32 36 40 42 48 64 72 80 96 128 192)
  set(icon_size "${icon_type}x${icon_type}")
  install(FILES grass-${icon_size}.png DESTINATION share/icons/hicolor/${icon_size}/apps)
endforeach()

install(FILES grass.svg
  DESTINATION share/icons/hicolor/scalable/apps
  RENAME grass71.svg)

install(FILES grass.appdata.xml DESTINATION share/appdata )
