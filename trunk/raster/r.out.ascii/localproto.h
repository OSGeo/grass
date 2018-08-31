int writeGRASSheader(FILE *);
int write_GRASS(int, FILE *, int, int, int, int, char *);

int writeMFheader(FILE *, int, int, int);
int write_MODFLOW(int, FILE *, int, int, int, int, int);

int writeGSheader(FILE *, const char *);
int write_GSGRID(int, FILE *, int, int, int, int, char *, int);
