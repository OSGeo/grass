#ifndef GREGION_LOCAL_PROTO_H
#define GREGION_LOCAL_PROTO_H

#define PRINT_REG    0x01
#define PRINT_SH     0x02
#define PRINT_LL     0x04
#define PRINT_EXTENT 0x08
#define PRINT_CENTER 0x10
#define PRINT_METERS 0x20
#define PRINT_3D     0x40
#define PRINT_MBBOX  0x80
#define PRINT_NANGLE 0x100

/* adjust.c */
int adjust_window(struct Cell_head *, int, int, int);

/* zoom.c */
int zoom(struct Cell_head *, char *, char *);

/* printwindow.c */
int print_window(struct Cell_head *, int);

#endif
