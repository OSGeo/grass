/*
* $Id$
*/

#ifndef _KEYFRAME_H
#define _KEYFRAME_H

/* these have to be 1 << KF_id_index */

#define KF_FROMX_MASK	0x00000001
#define KF_FROMY_MASK	0x00000002
#define KF_FROMZ_MASK	0x00000004
#define KF_FROM_MASK	0x00000007

#define KF_DIRX_MASK	0x00000008
#define KF_DIRY_MASK	0x00000010
#define KF_DIRZ_MASK	0x00000020
#define KF_DIR_MASK	0x00000038

#define KF_FOV_MASK	0x00000040
#define KF_TWIST_MASK	0x00000080

#define KF_ALL_MASK	0x000000FF

#define KF_NUMFIELDS 8 

#define KF_LINEAR 111
#define KF_SPLINE 222
#define KF_LEGAL_MODE(m) (m == KF_LINEAR || m == KF_SPLINE) 

#endif /* _KEYFRAME_H */
