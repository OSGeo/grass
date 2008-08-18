#include "driver.h"
#include "driverlib.h"

static struct {
    double t, b, l, r;
} window;

void COM_Set_window(double t, double b, double l, double r)
{
    window.t = t;
    window.b = b;
    window.l = l;
    window.r = r;

    if (driver->Set_window)
	(*driver->Set_window) (t, b, l, r);
}

void COM_Get_window(double *t, double *b, double *l, double *r)
{
    *t = window.t;
    *b = window.b;
    *l = window.l;
    *r = window.r;
}
