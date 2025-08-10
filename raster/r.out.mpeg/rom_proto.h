#define USE_PPM

#ifndef USE_PPM
void write_ycc(char *, char *, char *, int, int, int *, int *, char *);
#endif

void write_ppm(char *, char *, char *, int, int, int *, int *, char *);
void write_params(char *, char *[], char *, int, int, int, int, int);
void clean_files(char *, char *[], int);
