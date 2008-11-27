
/****************************************************************/
/*                                                              */
/*      This header file declares the global variables and the  */
/*      structures that are to be used for command line         */
/*      processing                                              */
/*                                                              */

/****************************************************************/

#ifndef CMD_LINE_H
#define CMD_LINE_H
extern char *backdrop_layer;
extern char *base_layer;
extern char *dir_layer;
extern char *max_layer;
extern char *spotdist_layer;
extern char *mois_layer;
extern char *out_layer;
extern char *start_layer;
extern char *velocity_layer;
extern char *x_out_layer;
extern char *y_out_layer;

extern float comp_dens;
extern int display;
extern int init_time;
extern int least;
extern int spotting;
extern int time_lag;
extern int x_out;
extern int y_out;
#endif /* CMD_LINE_H */
