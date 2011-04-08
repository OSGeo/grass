#ifndef GRASS_SEGMENT_H
#define GRASS_SEGMENT_H

#include <grass/gis.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

struct aq {			/* age queue */
    int cur;			/* segment number */
    struct aq *younger, *older;	/* pointer to next younger and next older */
} ;

typedef struct
{
    int open;			/* open flag */
    int nrows;			/* rows in original data */
    int ncols;			/* cols in original data */
    int len;			/* bytes per data value */
    int srows;			/* rows in segments */
    int scols;			/* cols in segments */
    int srowscols;   		/* rows x cols in segments */
    int size;			/* size in bytes of a segment */
    int spr;			/* segments per row */
    int spill;			/* cols in last segment in row */

    /* fast mode */
    int slow_adrs;      	/* toggles fast address mode */
    int scolbits;       	/* column bitshift */
    int srowbits;       	/* row bitshift */
    int segbits;        	/* segment bitshift */
    int slow_seek;      	/* toggles fast seek mode */
    int lenbits;        	/* data size bitshift */
    int sizebits;       	/* segment size bitshift */
    
    int fd;			/* file descriptor to read/write segment */
    struct SEGMENT_SCB		/* control blocks */
    {
	char *buf;		/* data buffer */
	char dirty;		/* dirty flag */
	struct aq *age;		/* pointer to position in age queue */
	int n;			/* segment number */
    } *scb;
    int *load_idx;		/* index of loaded segments */
    int nfreeslots;		/* number of free slots */
    int *freeslot;		/* array of free slots */
    struct aq *agequeue,	/* queue of age for order of access */ 
              *youngest, 	/* youngest in age queue */
	      *oldest;	        /* oldest in age queue */
    int nseg;			/* number of segments in memory */
    int cur;			/* last accessed segment */
    int offset;			/* offset of data past header */
} SEGMENT;

int segment_address(const SEGMENT *, int, int, int *, int *);
int segment_flush(SEGMENT *);
int segment_format(int, int, int, int, int, int);
int segment_format_nofill(int, int, int, int, int, int);
int segment_get(SEGMENT *, void *, int, int);
int segment_get_row(const SEGMENT *, void *, int);
int segment_init(SEGMENT *, int, int);
int segment_pagein(SEGMENT *, int);
int segment_pageout(SEGMENT *, int);
int segment_put(SEGMENT *, const void *, int, int);
int segment_put_row(const SEGMENT *, const void *, int);
int segment_release(SEGMENT *);
int segment_seek(const SEGMENT *, int, int);
int segment_setup(SEGMENT *);

#endif /* GRASS_SEGMENT_H */
