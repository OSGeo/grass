file(GLOB SRCICO "*.ico" "*.png")
file(GLOB SRCICONS "grass/*.png")
file(GLOB SRCFLAGS "flags/*.png")

add_custom_target(make_images_dir 
COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/gui/icons
COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/gui/flags
COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/docs/html/icons
COMMAND ${CMAKE_COMMAND} -E copy  ${SRCICO} ${CMAKE_CURRENT_BINARY_DIR}/gui/icons
COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols)

add_custom_target(copy_gui_icons
COMMAND ${CMAKE_COMMAND} -E copy  ${SRCICO} ${CMAKE_CURRENT_BINARY_DIR}/gui/icons/
DEPENDS make_images_dir)

add_custom_target(copy_gui_icons_grass
COMMAND ${CMAKE_COMMAND} -E copy  ${SRCICONS} ${CMAKE_CURRENT_BINARY_DIR}/gui/icons/grass
DEPENDS copy_gui_icons)

add_custom_target(copy_gui_icons_flags
COMMAND ${CMAKE_COMMAND} -E copy  ${SRCFLAGS} ${CMAKE_CURRENT_BINARY_DIR}/gui/icons/flags
DEPENDS copy_gui_icons_grass)

add_custom_target(copy_docs_html_icons
COMMAND ${CMAKE_COMMAND} -E copy  ${SRCICONS} ${CMAKE_CURRENT_BINARY_DIR}/docs/html/icons
DEPENDS copy_gui_icons_flags)

install(FILES ${SRCICO} DESTINATION gui/icons/)
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
