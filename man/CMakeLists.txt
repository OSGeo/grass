set(data_files
    grassdocs.css grass_logo.png grass_icon.png jquery.fixedheadertable.min.js
    parser_standard_options.js parser_standard_options.css)

file(COPY ${data_files} DESTINATION docs/html)
install(FILES ${data_files} DESTINATION docs/html)
