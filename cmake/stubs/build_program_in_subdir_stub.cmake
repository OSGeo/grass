#[[
AUTHOR(S):  Edouard Choinière
PURPOSE:    Gersemi CMake formatter stub for macro build_program_in_subdir
COPYRIGHT:  (C) 2026 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]
function(build_program_in_subdir dir_name)
    # gersemi: hints { SOURCES: sort+unique }
    # gersemi: hints { INCLUDES: sort+unique }
    # gersemi: hints { DEPENDS: sort+unique }
    # gersemi: hints { OPTIONAL_DEPENDS: sort+unique }
    # gersemi: hints { PRIMARY_DEPENDS: sort+unique }
    # gersemi: hints { DEFS: sort+unique }
    # gersemi: hints { HEADERS: sort+unique }
    # gersemi: hints { TEST_SOURCE: sort+unique }
    set(options EXE NO_DOCS)
    set(oneValueArgs
        NAME
        SRC_DIR
        SRC_REGEX
        RUNTIME_OUTPUT_DIR
        PACKAGE
        HTML_FILE_NAME
    )
    set(multiValueArgs
        SOURCES
        INCLUDES
        DEPENDS
        OPTIONAL_DEPENDS
        PRIMARY_DEPENDS
        DEFS
        HEADERS
        TEST_SOURCE
    )

    cmake_parse_arguments(
        G
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
endfunction()
