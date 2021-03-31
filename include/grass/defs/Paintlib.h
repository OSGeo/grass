#ifndef GRASS_PAINTLIBDEFS_H
#define GRASS_PAINTLIBDEFS_H

/* From applib */
/* alpha.c */
int Palpha(void);

/* close.c */
int Pclose(void);

/* colorlevel.c */
int Pcolorlevels(int *, int *, int *);

/* colormult.c */
int Pcolormultipliers(int *, int *, int *);

/* colornum.c */
int Pcolornum(double, double, double);

/* colortable.c */
int Pcolortable(unsigned char *, unsigned char *, unsigned char *,
		unsigned char *, int);
/* colorvalue.c */
int Pcolorvalue(int, float *, float *, float *);

/* connect.c */
int Pconnect(void);
int Pdisconnect(void);

/* data.c */
int Pdata_begin(void);
int Pdata(unsigned char *, int);
int Pdata_end(void);

/* device.c */
int P__closedev(void);
int P__errordev(char *);
int P__flushdev(void);
int P__opendev(char *, char *[], char *);
int P__readdev(void *, int);
int P__writedev(void *, int);

/* flush.c */
int Pflush(void);

/* hres.c */
double Phres(void);

/* io.c */
int P__opcode(int);
int P__get(char *, int);
int P__gets(char *);
int P__geti(void);
double P__getf(void);
int P__send(char *, int);
int P__sendi(int);
int P__sendf(double);
int P__sends(char *);
int P__transparent(int);

/* lock.c */
int Plock(void);

/* misc.c */
int Pblockspace(void);
int Pblocksize(void);
int Pnblocks(void);
int Ptextspace(void);
int Ptextfudge(void);
double Ptextscale(void);

/* nchars.c */
int Pnchars(void);

/* ncolors.c */
int Pncolors(void);

/* npixels.c */
int Pnpixels(int *, int *);

/* painter.c */
char *Ppainter_name(void);

/* pictsize.c */
int Ppictsize(int, int);

/* raster.c */
int Praster(void);

/* rle.c */
int Prle_begin(void);
int Prle_end(void);
int Prle_set_cols(int);

/* text.c */
int Ptext(char *);

/* vres.c */
double Pvres(void);

/* From Driver/{whatever} code */
/* init.c */
int Pinit(void);

/* finish.c */
int Pfinish(void);

/* From driverlib */
/* interface.c */
int paint_interface(int, char **);
int paint_error(char *);
int paint_delay(int);
int paint_lock(char *, int);
int paint_unlock(void);
int paint_colortable(int);

/* colors.c */
int Pset_color_levels(int);
int Pcolorlevels(int *, int *, int *);
int Pcolormultipliers(int *, int *, int *);
int Pcolornum(double, double, double);
int Pcolorvalue(int, float *, float *, float *);
int Pncolors(void);

/* io.c */
int Pclose(void);
int Pflush(void);
int Pout(char *, int);
int Poutc(int);
int Pouts(char *);

#ifdef PAINT_DRIVERLIB
int Popen(char *);
int Prle(unsigned char *, int);
#else
/* open.c */
int Popen(void);
int Prle(unsigned char, int);
#endif

#endif
