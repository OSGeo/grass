/* LIBDGL -- a Directed Graph Library implementation
 * SPDX-FileCopyrightText: 2002 Roberto Micarelli
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
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
extern void dgl_swapInt32Bytes(dglInt32_t *pn);
extern void dgl_swapInt64Bytes(dglInt64_t *pn);
extern int dgl_edge_prioritizer_del(dglGraph_s *pG, dglInt32_t nId,
                                    dglInt32_t nPriId);
extern int dgl_edge_prioritizer_add(dglGraph_s *pG, dglInt32_t nId,
                                    dglInt32_t nPriId);
extern void *dgl_reduce_edgeset(void *pvSet, int *pc, dglInt32_t nKey);

#endif
