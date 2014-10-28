
#include <grass/gis.h>
#include "path.h"
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
    html_polygon(&path);
}

void HTML_Stroke(void)
{
    path_reset(&path);
}

