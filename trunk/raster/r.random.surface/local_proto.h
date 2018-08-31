#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include "ransurf.h"

/* calcsd.c */
void CalcSD(void);

/* calcsurf.c */
void CalcSurface(void);

/* cpfilter.c */
void CopyFilter(FILTER * FPtr, FILTER Filter);

/* dd.c */
double DD(double Dist);

/* decay.c */
void DistDecay(double *Effect, int R, int C);

/* digits.c */
int Digits(double Double, int MaxSig);

/* gasdev.c */
double GasDev(void);

/* gennorm.c */
void GenNorm(void);

/* init.c */
void Init(void);

/* makebigf.c */
void MakeBigF(void);

/* makepp.c */
double MakePP(int Row, int Col, int OutRows, int OutCols,
	      double **Randoms, BIGF BigF);
/* random.c */
double ran1(void);

/* save.c */
void SaveMap(int NumMap, int MapSeed);

/* zero.c */
void ZeroMapCells(void);

#endif
