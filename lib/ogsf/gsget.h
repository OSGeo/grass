#ifndef _GSGET_H
#define _GSGET_H

#include <grass/ogsf.h>

#define GET_MAPATT1(buff, offset, att) att=(buff->ib? (float)buff->ib[offset]: \
				      buff->sb? (float)buff->sb[offset]: \
				      buff->cb? (float)buff->cb[offset]: \
				      buff->fb? (float)buff->fb[offset]: \
				      0.0)

#define GET_MAPATT2(buff, offset, att) att=(buff->ib? (float)buff->ib[offset]: \
				      buff->sb? (float)buff->sb[offset]: \
				      buff->cb? (float)buff->cb[offset]: \
				      buff->fb? (float)buff->fb[offset]: \
				      buff->k); \
				      if(buff->tfunc) \
					  att = (buff->tfunc)(att, offset);

/* cast to float, otherwise doesn't seem to handle neg. values */

#define SCALE_ATT(att, val, low, high)   ((val) <= att->max_nz && \
				(val) >= att->min_nz && att->range_nz? \
				(((val) - att->min_nz)/att->range_nz) * \
				((high) - (low)) + (low): 0)

#define GET_MAPATT(buff, offset, att) (get_mapatt(buff, offset, &(att)))

#define BM_GET_BYOFFSET(bm, off) (bm? BM_get(bm, (off%bm->cols), (off/bm->cols)): 0)



#define XYMAXPOS 0x3ff		/* 1023 */
#define ZMAXPOS 0x3ff		/* 1023 */

#define NXMASK 0xffe00000	/* top 11 bits */
#define NYMASK 0x1ffc00		/* middle 11 bits of packed int */
#define NZMASK 0x3ff		/* lowest 10 bits */

#define NZUP 0x000003ff

/* Fetch Normal vector from packed int */
/*
   #define FNORM(i,nv)  \
   nv[X] = ((int)(((i) & NXMASK) >> 21) - XYMAXPOS)/(float)XYMAXPOS; \
   nv[Y] = ((int)(((i) & NYMASK) >> 10) - XYMAXPOS)/(float)XYMAXPOS; \
   nv[Z] = (int)((i) & NZMASK) * GS_global_exag()/(float)ZMAXPOS
 */

#define FNORM(i,nv)  \
  nv[X] = ((int)(((i) & NXMASK) >> 21) - XYMAXPOS)/(float)XYMAXPOS; \
  nv[Y] = ((int)(((i) & NYMASK) >> 10) - XYMAXPOS)/(float)XYMAXPOS; \
  nv[Z] = (int)((i) & NZMASK) /(float)ZMAXPOS

/* Pack Normal vector into int */
#define PNORM(i,nv)  \
  i = ((unsigned int)((nv[X]*XYMAXPOS)+XYMAXPOS) << 21) |           \
  ((unsigned int)((nv[Y]*XYMAXPOS)+XYMAXPOS) << 10) |               \
  (unsigned int)(nv[Z]*ZMAXPOS)

#endif /* _GSGET_H */
