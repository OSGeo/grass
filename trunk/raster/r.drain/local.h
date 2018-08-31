struct metrics
{
    double ew_res, ns_res, diag_res;
};

void filldir(int, int, int, struct band3 *, struct metrics *);
void resolve(int, int, struct band3 *);
int dopolys(int, int, int, int);
void wtrshed(int, int, int, int, int);
void ppupdate(int, int, int, int, struct band3 *, struct band3 *);
