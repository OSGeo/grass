int read_dig(FILE *, struct Map_info *, struct Line **, int, int);
int read_att(FILE *, struct Categ **);
double ldist(double, double, struct Line *);
int old2new(char *, char *, int);
int new2old(char *, char *);
char dig_old_to_new_type(char);
int attributes(char *in, struct Map_info *Out);
