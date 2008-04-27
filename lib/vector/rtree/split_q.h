/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

/*-----------------------------------------------------------------------------
| Definitions and global variables.
-----------------------------------------------------------------------------*/

#define METHODS 1

#ifdef SPLIT_QC
#define SPLIT_Q_GLOBAL
#else
#define SPLIT_Q_GLOBAL extern
#endif

SPLIT_Q_GLOBAL struct Branch BranchBuf[MAXCARD+1];
SPLIT_Q_GLOBAL int BranchCount;
SPLIT_Q_GLOBAL struct Rect CoverSplit;
SPLIT_Q_GLOBAL RectReal CoverSplitArea;

/* variables for finding a partition */
SPLIT_Q_GLOBAL struct PartitionVars
{
	int partition[MAXCARD+1];
	int total, minfill;
	int taken[MAXCARD+1];
	int count[2];
	struct Rect cover[2];
	RectReal area[2];
} Partitions[METHODS];
