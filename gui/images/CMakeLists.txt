
# symbols
add_custom_target(copy_gui_symbols
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/basic ${CMAKE_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/demo ${CMAKE_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/extra ${CMAKE_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/geology ${CMAKE_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/legend ${CMAKE_BINARY_DIR}/gui/images/symbols/
COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/n_arrows ${CMAKE_BINARY_DIR}/gui/images/symbols/
DEPENDS copy_gui_images)

install(DIRECTORY basic DESTINATION gui/images/symbols/)
install(DIRECTORY demo DESTINATION gui/images/symbols/)
install(DIRECTORY extra DESTINATION gui/images/symbols/)
install(DIRECTORY geology DESTINATION gui/images/symbols/)
install(DIRECTORY legend DESTINATION gui/images/symbols/)
install(DIRECTORY n_arrows DESTINATION gui/images/symbols/)
