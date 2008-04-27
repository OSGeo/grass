# extra binaries and data to bundle into the GRASS.app package
# 
# use ${INSTALL_BIN} to copy binaries.
# use ${INSTALL_DATA} for headers and other non-executables.
# use ${MAKE_DIR_CMD} to create a subfolder if needed.
# use ${LN} and ${LN_DIR} to symlink files and folders respectively.
# 
# Destinations include: bin and lib and should be prefixed by
# ${MACOSX_BUNDLE_PREFIX}/
# Includes should not normally be needed.
# 
# For libraries, make sure to make symlinks if the symlink is the linked name.
# ie, libfoo.1.0.dylib is the library file, but libfoo.1.dylib is the link name:
#   ${LN} libfoo.1.0.dylib ${MACOSX_BUNDLE_PREFIX}/lib/libfoo.1.dylib
# If you're not sure, make all symlinks for a library.
# 
# use one line per file after the "bundle-macosx:" line, indented with a tab.
# ie:
#	${INSTALL_BIN} /usr/local/bin/gpsbabel ${MACOSX_BUNDLE_PREFIX}/bin
#	${INSTALL_BIN} /usr/local/pgsql/lib/libpq.5.0.dylib ${MACOSX_BUNDLE_PREFIX}/lib
#	${LN} libpq.5.0.dylib ${MACOSX_BUNDLE_PREFIX}/lib/libpq.5.dylib

bundle-macosx:
	@# add custom bundle commands here:
