
/****************************************************************/
/*                                                              */
/*      This header file declares the global variables and the  */
/*      structures that are to be used for command line         */
/*      processing                                              */
/*                                                              */

/****************************************************************/

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL double east;
GLOBAL double north;
GLOBAL double obs_elev;
GLOBAL double max_dist;
GLOBAL char *elev_layer;
GLOBAL char *patt_layer;
GLOBAL char *out_layer;
