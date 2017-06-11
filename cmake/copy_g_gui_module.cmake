# AUTHOR(S): Rashad Kanavath <rashad km gmail>
# PURPOSE:  Copy g.gui script plus .bat file if on windows
# COPYRIGHT: (C) 2020 by the GRASS Development Team
#   	    	 This program is free software under the GPL (>=v2)
#   	    	 Read the file COPYING that comes with GRASS for details.
# -DSOURCE_DIR
# -DGISBASE
# -DG_NAME
# -DSRC_SCRIPT_FILE
# -DBINARY_DIR

set(SCRIPT_EXT "")
if(WIN32)
  set(SCRIPT_EXT ".py")
endif()

if(WIN32)
  set(PGM_NAME ${G_NAME})
  configure_file(
    ${SOURCE_DIR}/cmake/windows_launch.bat.in
    ${GISBASE}/scripts/${G_NAME}.bat @ONLY)
endif(WIN32)

set(TMP_SCRIPT_FILE ${BINARY_DIR}/CMakeFiles/${G_NAME}${SCRIPT_EXT})
configure_file(${SRC_SCRIPT_FILE} ${TMP_SCRIPT_FILE} COPYONLY)
file(
  COPY ${TMP_SCRIPT_FILE}
  DESTINATION ${GISBASE}/scripts/
  FILE_PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)


file(REMOVE ${TMP_SCRIPT_FILE})
