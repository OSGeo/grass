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
    off_t nrows;		/* rows in original data */
    off_t ncols;		/* cols in original data */
    int len;			/* bytes per data value */
    int srows;			/* rows in segments */
    int scols;			/* cols in segments */
    int srowscols;   		/* rows x cols in segments */
    int size;			/* size in bytes of a segment */
    int spr;			/* segments per row */
    int spill;			/* cols in last segment in row */

    /* fast mode */
    int fast_adrs;      	/* toggles fast address mode */
    off_t scolbits;       	/* column bitshift */
    off_t srowbits;       	/* row bitshift */
    off_t segbits;        	/* segment bitshift */
    int fast_seek;      	/* toggles fast seek mode */
    int lenbits;        	/* data size bitshift */
    int sizebits;       	/* segment size bitshift */
    int (*address)();
    int (*seek)();
    
    char *fname;		/* segment file name */
    int fd;			/* file descriptor to read/write segment */
    struct scb			/* control blocks */
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

    char *cache;		/* all in memory cache */
} SEGMENT;

#include <grass/defs/segment.h>

#endif /* GRASS_SEGMENT_H */
