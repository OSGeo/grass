set(web_only_doc_files github_guide investigating_history style_guide)

add_custom_target(
    ADD_DOC_DEV_DOCS
    ALL
    COMMENT "Generate doc/development docs."
    DEPENDS LIB_PYTHON
)
set_target_properties(ADD_DOC_DEV_DOCS PROPERTIES FOLDER Docs)

generate_docs_list(
    TARGET ADD_DOC_DEV_DOCS
    DOC_FILES ${web_only_doc_files}
    MD_ONLY
)

unset(web_only_doc_files)
