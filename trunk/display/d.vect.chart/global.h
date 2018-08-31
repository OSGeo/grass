
typedef struct
{
    int none;
    int r, g, b;
} COLOR;

#define CTYPE_PIE  0
#define CTYPE_BAR  1

int plot(int ctype, struct Map_info *Map, int type, int field,
	 char *columns, int ncols, char *sizecol, int size, double scale,
	 COLOR * ocolor, COLOR * colors, int y_center,
	 double *max_reference, int do3d);
int pie(double cx, double cy, int size, double *val, int ncols,
	COLOR * ocolor, COLOR * colors, int do3d);
int bar(double cx, double cy, int size, double scale, double *val, int ncols,
	COLOR * ocolor, COLOR * colors, int y_center, double *max_reference,
	int do3d);
