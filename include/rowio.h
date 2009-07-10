#ifndef GRASS_ROWIO_H
#define GRASS_ROWIO_H

typedef struct
{
    int fd;			/* file descriptor for reading */
    int nrows;			/* number of rows to be held in memory */
    int len;			/* buffer length */
    int cur;			/* current row in memory */
    void *buf;			/* current data buf */
    int (*getrow) (int, void *, int, int);	/* routine to do the row reads */
    int (*putrow) (int, const void *, int, int);	/* routine to do the row writes */

    struct ROWIO_RCB
    {
	void *buf;		/* data buffer */
	int age;		/* for order of access */
	int row;		/* row number */
	int dirty;
    } *rcb;
} ROWIO;

int Rowio_fileno(const ROWIO *);
void Rowio_forget(ROWIO *, int);
void *Rowio_get(ROWIO *, int);
void Rowio_flush(ROWIO *);
int Rowio_put(ROWIO *, const void *, int);
void Rowio_release(ROWIO *);
int Rowio_setup(ROWIO *, int, int, int, int (*)(int, void *, int, int),
		int (*)(int, const void *, int, int));

#endif
