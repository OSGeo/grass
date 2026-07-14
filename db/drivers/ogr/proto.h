/*****************************************************************************
 *
 * MODULE:       OGR driver
 *
 * AUTHOR(S):    Radim Blazek
 *               Some updates by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      DB driver for OGR sources
 *
 * SPDX-FileCopyrightText: 2004-2009 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 *****************************************************************************/

/* cursor.c */
cursor *alloc_cursor(void);
void free_cursor(cursor *);

/* describe.c */
int describe_table(OGRLayerH, dbTable **, cursor *);

/* error.c */
void init_error(void);
