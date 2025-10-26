/*
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

#ifndef _DGL_dglGraph_s_H_
#define _DGL_dglGraph_s_H_

#ifdef DGL_STATS
#include <time.h>
#endif

#include "heap.h"
#include "tree.h"

/*
 * Graph State bitmask - returned by dglGet_State() function
 */
#define DGL_GS_FLAT                0x1 /* otherwise is TREE */

/*
 * Graph Family
 */
#define DGL_GF_COMPLETE            0x1
#define DGL_GF_BIPARTITE           0x2
#define DGL_GF_REGULAR             0x4
#define DGL_GF_BOUQUET             0x8
#define DGL_GF_DIPOLE              0x10
#define DGL_GF_PATH                0x20
#define DGL_GF_CYCLE               0x40

/*
 * Graph Options
 */
#define DGL_GO_EdgePrioritize_COST 0x10
#define DGL_GO_EdgePrioritize_ATTR 0x20
#define DGL_GO_NodePrioritize_ATTR 0x40

/*
 * Node Status bitmask - returned by dglNodeGet_Status()
 */
#define DGL_NS_HEAD                0x1 /* node exists as at least one edge's head (static) */
#define DGL_NS_TAIL                0x2 /* node exists as at least one edge's tail (static) */
#define DGL_NS_ALONE               0x4 /* node is a component */

/*
 * Edge Status bitmask - returned by dglEdgeGet_Status()
 */
#define DGL_ES_DIRECTED            0x1 /* force edge to be directed */

/*
 * Endianness Values - returned by dglGet_Endianess() function
 */
#define DGL_ENDIAN_BIG             1
#define DGL_ENDIAN_LITTLE          2

/*
 * miscellaneous
 */
/* add-edge/add-node flags */
#define DGL_STRONGCONNECT          0x1
#define DGL_ALONE                  0x2
#define DGL_MERGE_EDGE             0x4
/* */

/*
 * Shortest Path clip definitions
 */
typedef struct _dglSPClipInput {
    dglInt32_t *pnPrevEdge;
    dglInt32_t *pnNodeFrom;
    dglInt32_t *pnEdge;
    dglInt32_t *pnNodeTo;
    dglInt32_t nFromDistance;

} dglSPClipInput_s;

typedef struct _dglSPClipOutput {
    dglInt32_t nEdgeCost;

} dglSPClipOutput_s;

/*
 * Spanning clip definitions
 */
typedef struct _dglSpanClipInput {
    dglInt32_t *pnNodeFrom;
    dglInt32_t *pnEdge;
    dglInt32_t *pnNodeTo;

} dglSpanClipInput_s;

typedef struct _dglSpanClipOutput {
    dglInt32_t *pnReserved;

} dglSpanClipOutput_s;

struct dglGraph;

/*
 * Node Prioritizer
 */
typedef struct {
    void *pvAVL;
} dglNodePrioritizer_s;

/*
 * Edge Prioritizer
 */
typedef struct {
    int cEdge;
    int iEdge;
    dglTreeEdgePri32_s *pEdgePri32Item;
    void *pvAVL;
} dglEdgePrioritizer_s;

/*
 * The graph context
 */
typedef struct _dglGraph {
    int iErrno;

    dglByte_t Version;
    dglByte_t Endian;
    dglInt32_t NodeAttrSize;
    dglInt32_t EdgeAttrSize;
    dglInt32_t aOpaqueSet[16];

    dglInt32_t cNode;
    dglInt32_t cHead;
    dglInt32_t cTail;
    dglInt32_t cAlone;
    dglInt32_t cEdge;
    dglInt64_t nnCost;

    dglInt32_t Flags;
    dglInt32_t nFamily;
    dglInt32_t nOptions;

    void *pNodeTree;
    void *pEdgeTree;
    dglByte_t *pNodeBuffer;
    dglInt32_t iNodeBuffer;
    dglByte_t *pEdgeBuffer;
    dglInt32_t iEdgeBuffer;

    dglEdgePrioritizer_s edgePrioritizer;
    dglNodePrioritizer_s nodePrioritizer;

    /* so far statistics are only computed by dglAddEdge() */
#ifdef DGL_STATS
    clock_t clkAddEdge;  /* cycles spent during the last addedge execution */
    int cAddEdge;        /* # of calls to dglAddEdge() */
    clock_t clkNodeTree; /* cycles spent in accessing the node binary tree */
    int cNodeTree;       /* # of probes in the node tree */
#endif
} dglGraph_s;

/*
 * Shortest Path clip function type
 */
typedef int (*dglSPClip_fn)(dglGraph_s *, dglSPClipInput_s *,
                            dglSPClipOutput_s *, void *);

/*
 * Spanning clip function type
 */
typedef int (*dglSpanClip_fn)(dglGraph_s *, dglGraph_s *, dglSpanClipInput_s *,
                              dglSpanClipOutput_s *, void *);

/*
 * An ARC defined as : from-node, to-node, edge pointer, to-node-distance (from
 * the path starting node)
 */
typedef struct _dglSPArc {
    dglInt32_t nFrom;
    dglInt32_t nTo;
    dglInt32_t *pnEdge;
    dglInt32_t nDistance;
} dglSPArc_s;

/*
 * Shortest Path Report
 */
typedef struct _dglSPReport {
    dglInt32_t nStartNode;
    dglInt32_t nDestinationNode;
    dglInt32_t nDistance;
    dglInt32_t cArc;
    dglSPArc_s *pArc;
} dglSPReport_s;

/*
 * Shortest Path Cache
 */
typedef struct {
    dglInt32_t nStartNode;
    dglHeap_s NodeHeap;
    void *pvVisited;
    void *pvPredist;
} dglSPCache_s;

/*
 * Node Traverser
 */
typedef struct {
    dglGraph_s *pGraph;
    void *pvAVLT;
    dglInt32_t *pnNode;
} dglNodeTraverser_s;

/*
 * Edgeset Traverser
 */
typedef struct {
    dglGraph_s *pGraph;
    dglInt32_t *pnEdgeset;
    void *pvCurrentItem;
    int cEdge, iEdge;
} dglEdgesetTraverser_s;

/*
 * Edge Traverser
 */
typedef struct {
    dglGraph_s *pGraph;
    void *pvAVLT;
    dglInt32_t *pnEdge;
    dglEdgePrioritizer_s *pEdgePrioritizer;
} dglEdgeTraverser_s;

/*
 * Error codes returned by dglError
 */
#define DGL_ERR_BadVersion            1
#define DGL_ERR_BadNodeType           2
#define DGL_ERR_MemoryExhausted       3
#define DGL_ERR_HeapError             4
#define DGL_ERR_UndefinedMethod       5
#define DGL_ERR_Write                 6
#define DGL_ERR_Read                  7
#define DGL_ERR_NotSupported          8
#define DGL_ERR_UnknownByteOrder      9
#define DGL_ERR_HeadNodeNotFound      10
#define DGL_ERR_TailNodeNotFound      11
#define DGL_ERR_BadEdge               12
#define DGL_ERR_BadOnFlatGraph        13
#define DGL_ERR_BadOnTreeGraph        14
#define DGL_ERR_NodeNotFound          15
#define DGL_ERR_TreeSearchError       16
#define DGL_ERR_UnexpectedNullPointer 17
#define DGL_ERR_VersionNotSupported   18
#define DGL_ERR_EdgeNotFound          19
#define DGL_ERR_NodeAlreadyExist      20
#define DGL_ERR_NodeIsAComponent      21
#define DGL_ERR_EdgeAlreadyExist      22
#define DGL_ERR_BadArgument           23

/*
 * graph context management
 */
int dglInitialize(dglGraph_s *pGraph, dglByte_t Version,
                  dglInt32_t NodeAttrSize, dglInt32_t EdgeAttrSize,
                  dglInt32_t *pOpaqueSet);
int dglRelease(dglGraph_s *pGraph);
int dglUnflatten(dglGraph_s *pGraph);
int dglFlatten(dglGraph_s *pGraph);
void dglResetStats(dglGraph_s *pgraph);

/*
 * node management
 */
dglInt32_t *dglGetNode(dglGraph_s *pGraph, dglInt32_t nNodeId);
int dglAddNode(dglGraph_s *pGraph, dglInt32_t nNodeId, void *pvNodeAttr,
               dglInt32_t nFlags);
int dglDelNode(dglGraph_s *pGraph, dglInt32_t nNodeId);
dglInt32_t dglNodeGet_Id(dglGraph_s *pGraph, dglInt32_t *pnNode);
dglInt32_t *dglNodeGet_OutEdgeset(dglGraph_s *pGraph, dglInt32_t *pnNode);
dglInt32_t *dglNodeGet_InEdgeset(dglGraph_s *pGraph, dglInt32_t *pnNode);
dglInt32_t dglNodeGet_Status(dglGraph_s *pGraph, dglInt32_t *pnNode);
dglInt32_t *dglNodeGet_Attr(dglGraph_s *pGraph, dglInt32_t *pnNode);
void dglNodeSet_Attr(dglGraph_s *pGraph, dglInt32_t *pnNode,
                     dglInt32_t *pnAttr);
int dglNodeGet_InDegree(dglGraph_s *pGraph, dglInt32_t *pnNode);
int dglNodeGet_OutDegree(dglGraph_s *pGraph, dglInt32_t *pnNode);
int dglNodeGet_Valence(dglGraph_s *pGraph, dglInt32_t *pnNode);

/*
 * edge management
 */
dglInt32_t dglEdgesetGet_EdgeCount(dglGraph_s *pGraph,
                                   dglInt32_t *pnOutEdgeset);

dglInt32_t dglEdgeGet_Id(dglGraph_s *pGraph, dglInt32_t *pnEdge);
dglInt32_t dglEdgeGet_Cost(dglGraph_s *pGraph, dglInt32_t *pnEdge);
dglInt32_t *dglEdgeGet_Head(dglGraph_s *pGraph, dglInt32_t *pnEdge);
dglInt32_t *dglEdgeGet_Tail(dglGraph_s *pGraph, dglInt32_t *pnEdge);
dglInt32_t *dglEdgeGet_Attr(dglGraph_s *pGraph, dglInt32_t *pnEdge);
int dglEdgeSet_Attr(dglGraph_s *pGraph, dglInt32_t *pnAttr, dglInt32_t *pnEdge);

dglInt32_t *dglGetEdge(dglGraph_s *pGraph, dglInt32_t nEdgeId);

int dglDelEdge(dglGraph_s *pGraph, dglInt32_t nEdgeId);

int dglAddEdge(dglGraph_s *pGraph, dglInt32_t nHead, dglInt32_t nTail,
               dglInt32_t nCost, dglInt32_t nEdge);

int dglAddEdgeX(dglGraph_s *pGraph, dglInt32_t nHead, dglInt32_t nTail,
                dglInt32_t nCost, dglInt32_t nEdge, void *pvFnodeAttr,
                void *pvTnodeAttr, void *pvEdgeAttr, dglInt32_t nFlags);

/*
 * graph I/O
 */
int dglWrite(dglGraph_s *pGraph, int fd);
int dglRead(dglGraph_s *pGraph, int fd);

typedef struct {
    dglGraph_s *pG;
    int nState;
    int fSwap;
    int cb;
    int ib;
    unsigned char *pb;
    unsigned char ab[118]; /* 118 = graph header size */
} dglIOContext_s;

int dglIOContextInitialize(dglGraph_s *, dglIOContext_s *);
void dglIOContextRelease(dglIOContext_s *);

/*
 * Chunked Write callback function type
 */
typedef int (*dglWriteChunk_fn)(dglGraph_s *, unsigned char *pbChunk,
                                int cbChunk, void *pvArg);

int dglWriteChunk(dglIOContext_s *, dglWriteChunk_fn, void *pvArg);
int dglReadChunk(dglIOContext_s *, dglByte_t *pbChunk, int cbChunk);

/*
 * Algorithms
 */
int dglShortestPath(dglGraph_s *pGraph, dglSPReport_s **ppReport,
                    dglInt32_t nStartNode, dglInt32_t nDestinationNode,
                    dglSPClip_fn fnClip, void *pvClipArg, dglSPCache_s *pCache);
int dglShortestPathGraph(dglGraph_s *pGraph, dglGraph_s *pGraphOut,
                         dglInt32_t nStartNode, dglInt32_t nDestinationNode,
                         dglSPClip_fn fnClip, void *pvClipArg,
                         dglSPCache_s *pCache);
int dglShortestDistance(dglGraph_s *pGraph, dglInt32_t *pnDistance,
                        dglInt32_t nStartNode, dglInt32_t nDestinationNode,
                        dglSPClip_fn fnClip, void *pvClipArg,
                        dglSPCache_s *pCache);
int dglShortestDistanceGraph(dglGraph_s *pGraph, dglGraph_s *pGraphOut,
                             dglInt32_t nStartNode, dglInt32_t nDestinationNode,
                             dglSPClip_fn fnClip, void *pvClipArg,
                             dglSPCache_s *pCache);

int dglInitializeSPCache(dglGraph_s *pgraph, dglSPCache_s *pCache);
void dglReleaseSPCache(dglGraph_s *pgraph, dglSPCache_s *pCache);
void dglFreeSPReport(dglGraph_s *pGraph, dglSPReport_s *pSPReport);

int dglDepthSpanning(dglGraph_s *pgraphInput, dglGraph_s *pgraphOutput,
                     dglInt32_t nVertexNode, dglSpanClip_fn fnClip,
                     void *pvClipArg);

int dglDepthComponents(dglGraph_s *pgraphInput, dglGraph_s *pgraphComponents,
                       int cgraphComponents, dglSpanClip_fn fnClip,
                       void *pvClipArg);

int dglMinimumSpanning(dglGraph_s *pgraphInput, dglGraph_s *pgraphOutput,
                       dglInt32_t nVertexNode, dglSpanClip_fn fnClip,
                       void *pvClipArg);

/*
 * error management
 */
int dglErrno(dglGraph_s *pgraph);
char *dglStrerror(dglGraph_s *pgraph);

/*
 * graph property hiders
 */
int dglGet_Version(dglGraph_s *pGraph);
void dglSet_Version(dglGraph_s *pGraph, int Version);
int dglGet_Endianess(dglGraph_s *pGraph);
int dglGet_NodeAttrSize(dglGraph_s *pGraph);
int dglGet_EdgeAttrSize(dglGraph_s *pGraph);
int dglGet_NodeCount(dglGraph_s *pGraph);
int dglGet_HeadNodeCount(dglGraph_s *pGraph);
int dglGet_TailNodeCount(dglGraph_s *pGraph);
int dglGet_AloneNodeCount(dglGraph_s *pGraph);
int dglGet_EdgeCount(dglGraph_s *pGraph);
int dglGet_State(dglGraph_s *pGraph);
dglInt32_t *dglGet_Opaque(dglGraph_s *pGraph);
void dglSet_Opaque(dglGraph_s *pGraph, dglInt32_t *pOpaque);
int dglGet_NodeSize(dglGraph_s *pGraph);
int dglGet_EdgeSize(dglGraph_s *pGraph);
dglInt64_t dglGet_Cost(dglGraph_s *pGraph);
void dglSet_Cost(dglGraph_s *pGraph, dglInt64_t nnCost);
dglInt32_t dglGet_Family(dglGraph_s *pGraph);
void dglSet_Family(dglGraph_s *pGraph, dglInt32_t nFamily);
dglInt32_t dglGet_Options(dglGraph_s *pGraph);
void dglSet_Options(dglGraph_s *pGraph, dglInt32_t nOptions);
dglEdgePrioritizer_s *dglGet_EdgePrioritizer(dglGraph_s *pGraph);
dglNodePrioritizer_s *dglGet_NodePrioritizer(dglGraph_s *pGraph);

/*
 * node traverser
 */
int dglNode_T_Initialize(dglNodeTraverser_s *pTraverser, dglGraph_s *pGraph);
void dglNode_T_Release(dglNodeTraverser_s *pTraverser);
dglInt32_t *dglNode_T_First(dglNodeTraverser_s *pTraverser);
dglInt32_t *dglNode_T_Last(dglNodeTraverser_s *pTraverser);
dglInt32_t *dglNode_T_Next(dglNodeTraverser_s *pTraverser);
dglInt32_t *dglNode_T_Prev(dglNodeTraverser_s *pTraverser);
dglInt32_t *dglNode_T_Find(dglNodeTraverser_s *pTraverser, dglInt32_t nNodeId);

/*
 * edgeset traverser
 */
int dglEdgeset_T_Initialize(dglEdgesetTraverser_s *pTraverser,
                            dglGraph_s *pGraph, dglInt32_t *pnEdgeset);
void dglEdgeset_T_Release(dglEdgesetTraverser_s *pTraverser);
dglInt32_t *dglEdgeset_T_First(dglEdgesetTraverser_s *pTraverser);
dglInt32_t *dglEdgeset_T_Next(dglEdgesetTraverser_s *pTraverser);

/*
 * edge traverser
 */
int dglEdge_T_Initialize(dglEdgeTraverser_s *pTraverser, dglGraph_s *pGraph,
                         dglEdgePrioritizer_s *pEdgePrioritizer);
void dglEdge_T_Release(dglEdgeTraverser_s *pTraverser);
dglInt32_t *dglEdge_T_First(dglEdgeTraverser_s *pTraverser);
dglInt32_t *dglEdge_T_Next(dglEdgeTraverser_s *pTraverser);

#endif
