/* LIBDGL -- a Directed Graph Library implementation
 * Copyright (C) 2002 Roberto Micarelli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * best view tabstop=4
 */

#ifndef _DGL_HELPERS_H_
#define _DGL_HELPERS_H_

#include "tree.h"

extern unsigned char *dgl_mempush(unsigned char *pstack, long *istack,
				  long size, void *pv);
extern unsigned char *dgl_mempop(unsigned char *pstack, long *istack,
				 long size);
extern void dgl_swapInt32Bytes(dglInt32_t * pn);
extern void dgl_swapInt64Bytes(dglInt64_t * pn);
extern int dgl_edge_prioritizer_del(dglGraph_s * pG, dglInt32_t nId,
				    dglInt32_t nPriId);
extern int dgl_edge_prioritizer_add(dglGraph_s * pG, dglInt32_t nId,
				    dglInt32_t nPriId);
extern void *dgl_reduce_edgeset(void *pvSet, int *pc, dglInt32_t nKey);

#endif
