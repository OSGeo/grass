# images
file(GLOB png_files "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
install(FILES ${png_files} DESTINATION gui/images/)
# symbols
foreach(category "basic" "demo" "extra" "geology" "legend" "n_arrows")
  file(GLOB cat_png_files "${CMAKE_CURRENT_SOURCE_DIR}/${category}/*.png")
  install(FILES ${cat_png_files} DESTINATION gui/images/symbols/${category}/)
endforeach()
