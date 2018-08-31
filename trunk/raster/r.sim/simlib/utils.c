/* output.c (simlib), 20.nov.2002, JH */

#include <grass/waterglobs.h>

double amax1(double arg1, double arg2)
{
    double res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}

double amin1(double arg1, double arg2)
{
    double res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}

int min(int arg1, int arg2)
{
    int res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}

int max(int arg1, int arg2)
{
    int res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }

    return res;
}

