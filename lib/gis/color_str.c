#include "string.h"
#include <grass/gis.h>
#include <grass/colors.h>

/* The order in this table is important! It will be indexed by color number */
static const struct color_rgb standard_colors_rgb[] =
{
  {  0,  0,  0}, /* This is a dummy value to make lookup easier */
  {  0,  0,  0}, /* BLACK   */
  {255,  0,  0}, /* RED     */
  {  0,255,  0}, /* GREEN   */
  {  0,  0,255}, /* BLUE    */
  {255,255,  0}, /* YELLOW  */
  {  0,255,255}, /* CYAN    */
  {255,  0,255}, /* MAGENTA */
  {255,255,255}, /* WHITE   */
  {128,128,128}, /* GRAY    */
  {255,128,  0}, /* ORANGE  */
  {100,128,255}, /* AQUA    */
  {  0,128,255}, /* INDIGO  */
  {128,  0,255}, /* VIOLET  */
  {180, 77, 25}  /* BROWN   */
};

/* The order in this table has no meaning. */
static const struct color_name standard_color_names[] =
{
    {"black",   BLACK},
    {"red",     RED},
    {"green",   GREEN},
    {"blue",    BLUE},
    {"yellow",  YELLOW},
    {"cyan",    CYAN},
    {"magenta", MAGENTA},
    {"white",   WHITE},
    {"grey",    GREY},
    {"gray",    GRAY},
    {"orange",  ORANGE},
    {"aqua",    AQUA},
    {"indigo",  INDIGO},
    {"violet",  VIOLET},
    {"purple",  PURPLE},
    {"brown",   BROWN}
};

int G_num_standard_colors(void)
{
    return sizeof(standard_colors_rgb) / sizeof(standard_colors_rgb[0]);
}

struct color_rgb G_standard_color_rgb(int n)
{
    return standard_colors_rgb[n];
}

int G_num_standard_color_names(void)
{
    return sizeof(standard_color_names) / sizeof(standard_color_names[0]);
}

const struct color_name *G_standard_color_name(int n)
{
    return &standard_color_names[n];
}

/* 
*  Parses color string and sets red,green,blue
* 
*  Returns: 1 - OK
*           2 - NONE 
*           0 - Error 
* 
*/
int G_str_to_color(const char *str, int *red, int *grn, int *blu)
{
    char buf[100]; 
    int num_names = G_num_standard_color_names();
    int i;

    G_strcpy(buf, str);
    G_chop(buf);
    
    G_debug(3, "G_str_to_color(): str = '%s'", buf);

    if (G_strcasecmp( buf, "NONE" ) == 0)
	return 2;
   
    if (sscanf(buf, "%d%*[,:; ]%d%*[,:; ]%d", red, grn, blu) == 3)
    {
	if (*red < 0 || *red > 255 ||
	    *grn < 0 || *grn > 255 ||
	    *blu < 0 || *blu > 255)
	    return 0;

        return 1;
    }

    /* Look for this color in the standard (preallocated) colors */
    for (i = 0; i < num_names; i++)
    {
	const struct color_name *name = &standard_color_names[i];

	if (G_strcasecmp(buf, name->name) == 0)
	{
	    struct color_rgb rgb = standard_colors_rgb[name->number];

	    *red = (int) rgb.r;
	    *grn = (int) rgb.g;
	    *blu = (int) rgb.b;

	    return 1;
	}
    }
	
    return 0;
}

