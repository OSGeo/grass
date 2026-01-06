# work in progress...


file(GLOB doc_MDFILES "${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/source/*.md")

foreach(md_file ${doc_MDFILES})
  get_filename_component(PGM_NAME ${md_file} NAME)
  add_custom_command(
    TARGET create_man_pages
    PRE_BUILD
    COMMAND ${MD2MAN} ${OUTDIR}/${GRASS_INSTALL_MKDOCSDIR}/${PGM_NAME}.html
            ${OUTDIR}/${GRASS_INSTALL_MANDIR}/${PGM_NAME}.1
  )
endforeach()

#[[
COMMAND ${HTML2MAN} ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${PGM_NAME}.html
        ${OUTDIR}/${GRASS_INSTALL_MANDIR}/${PGM_NAME}.1
]]
