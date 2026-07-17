#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Shortcut macro to call build_module with EXE argument set
SPDX-FileCopyrightText: 2020 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later
#]]

macro(build_program)
    # Ignoring this definition since using a stub as the definition instead
    # gersemi: ignore
    build_module(${ARGN} EXE)
endmacro()
