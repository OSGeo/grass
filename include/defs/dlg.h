#ifndef GRASS_DLGDEFS_H
#define GRASS_DLGDEFS_H

int dlg_init(FILE *, struct dlg *);
int dlg_read(FILE *, struct dlg *);
int _dlg_read_node(struct dlg_node *, FILE *);
int _dlg_read_area(struct dlg_area *, FILE *);
int _dlg_read_line(struct dlg_line *, FILE *);
int _dlg_write_area(struct dlg_area *, FILE *);
int dlg_write_header(FILE *, struct dlg *);
int _dlg_write_line(struct dlg_line *, FILE *);
int _dlg_write_node(struct dlg_node *, FILE *);
int dlg_read_whole_area(FILE *, struct dlg *, int, double **, double **,
			int *, int *);
int dlg_read_area(FILE *, struct dlg *, int);
int dlg_read_line(FILE *, struct dlg *, int);
int dlg_read_node(FILE *, struct dlg *, int);
int dlg_read_int(FILE *, int, int **);
int dlg_write_int(FILE *, int, int *);
int dlg_write_double(FILE *, int, double *);
int dlg_write_area(FILE *, struct dlg *, int);
int dlg_write_line(FILE *, struct dlg *, int);
int dlg_write_node(FILE *, struct dlg *, int);

#endif
