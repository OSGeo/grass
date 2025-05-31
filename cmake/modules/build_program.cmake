#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Shortcut macro to call build_module with EXE argument set
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

macro(build_program)
  build_module(${ARGN} EXE)
endmacro()
