# symbols

set(images_stamp_file ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/GUI_IMAGES.stamp)

add_custom_command(OUTPUT ${images_stamp_file}
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/symbols/ ${GISBASE}/gui/images/
COMMAND ${CMAKE_COMMAND} -E touch ${images_stamp_file}
)

add_custom_target(copy_gui_images DEPENDS copy_gui_icons ${images_stamp_file})

install(DIRECTORY basic DESTINATION gui/images/symbols/)
install(DIRECTORY demo DESTINATION gui/images/symbols/)
install(DIRECTORY extra DESTINATION gui/images/symbols/)
install(DIRECTORY geology DESTINATION gui/images/symbols/)
install(DIRECTORY legend DESTINATION gui/images/symbols/)
install(DIRECTORY n_arrows DESTINATION gui/images/symbols/)
