#ifndef GRASS_ROWIODEFS_H
#define GRASS_ROWIODEFS_H

int Rowio_fileno(const ROWIO *);
void Rowio_forget(ROWIO *, int);
void *Rowio_get(ROWIO *, int);
void Rowio_flush(ROWIO *);
int Rowio_put(ROWIO *, const void *, int);
void Rowio_release(ROWIO *);
int Rowio_setup(ROWIO *, int, int, int, int (*)(int, void *, int, int),
		int (*)(int, const void *, int, int));

#endif
