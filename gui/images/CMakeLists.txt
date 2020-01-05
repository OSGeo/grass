# images
file(GLOB png_files "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
install(FILES ${png_files} DESTINATION gui/images )
# symbols
add_custom_target(copy_gui_images
COMMAND ${CMAKE_COMMAND} -E copy  ${png_files} ${CMAKE_CURRENT_BINARY_DIR}/gui/images
DEPENDS make_images_dir )

add_custom_target(copy_gui_symbols
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/basic ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/demo ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/extra ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/geology ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/legend ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/n_arrows ${CMAKE_CURRENT_BINARY_DIR}/gui/images/symbols/
DEPENDS make_images_dir)

install(DIRECTORY basic DESTINATION gui/images/symbols/)
install(DIRECTORY demo DESTINATION gui/images/symbols/)
install(DIRECTORY extra DESTINATION gui/images/symbols/)
install(DIRECTORY geology DESTINATION gui/images/symbols/)
install(DIRECTORY legend DESTINATION gui/images/symbols/)
install(DIRECTORY n_arrows DESTINATION gui/images/symbols/)
