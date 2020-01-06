#TODO
add_subdirectory(icons)
# images
#file(GLOB gui_images "${CMAKE_CURRENT_SOURCE_DIR}/images/*.png")
#install(FILES ${gui_images} DESTINATION gui/images )

add_custom_target(copy_gui_images
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_BINARY_DIR}/gui/
DEPENDS copy_gui_icons_flags)

install(DIRECTORY gui/images DESTINATION gui)

#add_subdirectory(images)

#add_subdirectory(scripts)

add_subdirectory(wxpython)

install(FILES xml/grass-interface.dtd  DESTINATION gui/xml)
