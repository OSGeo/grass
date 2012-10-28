#ifndef __GIS_LOCAL_PROTO_H__
#define __GIS_LOCAL_PROTO_H__

/* subroutines used only by GIS Library */
/* TODO: move other G__*() subroutines here */

/* location.c */
char *G__location_path(void);

/* mapset.c */
const char *G__mapset(void);
char *G__mapset_path(void);

#endif
