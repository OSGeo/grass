
#include <grass/gis.h>
#include "driverlib.h"
#include "htmlmap.h"

static struct path path;

void HTML_Begin(void)
{
    path_begin(&path);
}

void HTML_Move(double x, double y)
{
    path_move(&path, x, y);
}

void HTML_Cont(double x, double y)
{
    path_cont(&path, x, y);
}

void HTML_Close(void)
{
    path_close(&path);
}

void HTML_Fill(void)
{
    path_fill(&path, html_polygon);
}

void HTML_Stroke(void)
{
    path_reset(&path);
}

