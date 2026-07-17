/* Header file: border.h
 *
 *  AUTHOR:      M. Hamish Bowman, New Zealand  February 2007
 *
 *  SPDX-FileCopyrightText: 2007 Hamish Bowman
 *  SPDX-FileCopyrightText: Other GRASS authors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

struct border {
    double r, g, b;
    double width;
};

extern struct border brd;
