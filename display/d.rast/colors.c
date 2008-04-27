#include <string.h>
#include <grass/gis.h>

char *color_list (void)
{
    const char *color;
    int n;
    static char list[1024];

    *list = 0;
    for (n = 0; color = G_color_name(n); n++)
    {
	if(n) strcat(list,",");
	strcat(list,color);
    }
    return list;
}

int get_rgb (char *color, int *r, int *g, int *b)
{
    float fr, fg, fb;

    G_color_values (color, &fr, &fg, &fb);

    *r = (int) (fr * 255);
    *b = (int) (fb * 255);
    *g = (int) (fg * 255);

    return 0;
}
