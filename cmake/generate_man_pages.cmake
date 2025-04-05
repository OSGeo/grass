# work in progress...


file(GLOB doc_HTMLFILES "${OUTDIR}/${GRASS_INSTALL_DOCDIR}/*.html")

foreach(html_file ${doc_HTMLFILES})
  get_filename_component(PGM_NAME ${html_file} NAME)
  add_custom_command(
    TARGET create_man_pages
    PRE_BUILD
    COMMAND ${HTML2MAN} ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${PGM_NAME}.html
            ${OUTDIR}/${GRASS_INSTALL_MANDIR}/${PGM_NAME}.1
  )
endforeach()

#[[
COMMAND ${HTML2MAN} ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${PGM_NAME}.html
        ${OUTDIR}/${GRASS_INSTALL_MANDIR}/${PGM_NAME}.1
]]
