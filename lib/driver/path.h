
#ifndef DRIVERLIB_PATH_H
#define DRIVERLIB_PATH_H

enum path_mode {
    P_MOVE,
    P_CONT,
    P_CLOSE,
};

struct vertex {
    double x, y;
    int mode;
};

struct path {
    struct vertex *vertices;
    int count;
    int alloc;
    int start;
};

void path_init(struct path *);
void path_free(struct path *);
void path_alloc(struct path *, int);
void path_reset(struct path *);
void path_append(struct path *, double, double, int);
void path_copy(struct path *, const struct path *);
void path_begin(struct path *);
void path_move(struct path *, double, double);
void path_cont(struct path *, double, double);
void path_close(struct path *);
void path_stroke(struct path *, void (*)(double, double, double, double));

#endif

