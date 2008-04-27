/* Function: read_scalebar
**
** Author: Paul W. Carlson	April 1992
*/

#include <stdlib.h>
#include <string.h>
#include "decorate.h"
#include "ps_info.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] =
{
    "where      x y",
    "length	length",
    "height	height",
    "segment	no_segemnts",
    "numbers	no_labels",
    "fontsize   fontsize",
    "background [Y|n]",
    ""
};

int 
read_scalebar (void)
{
   char buf[1024];
   char *key, *data;
   char ch;

   /* struct defined in decorate.h */
   sb.segment = 4; /* four segments */
   sb.numbers = 1; /* label each segment */
   sb.fontsize = 8; 
   sb.width = 1.;
   sb.length = -1.;
   sb.height = 0.1; /* default height in inches */
   sb.x = PS.page_width/2.;
   sb.y = 2.;
   sb.bgcolor = 1; /* default is "on" [white|none] (TODO: multi-color) */


    while (input(2, buf, help))
    {
	if (!key_data(buf, &key, &data)) continue;

        if (KEY("where"))
 	{
	    if (sscanf(data, "%lf %lf", &sb.x, &sb.y) != 2)
	    {
		error(key, data, "illegal where request");
	    }
	    else continue;
	}

        if (KEY("height"))
 	{
	    if (sscanf(data, "%lf", &sb.height) != 1 || sb.height <= 0. )
	    {
		error(key, data, "illegal height request");
	    }
	    else continue;
	}
	
	if (KEY("length"))
 	{
	    if (sscanf(data, "%lf", &sb.length) != 1 || sb.length <= 0. )
	    {
		error(key, data, "illegal length request");
	    }
	    else continue;
	}
	
	if (KEY("segment"))
 	{
	    if (sscanf(data, "%d", &sb.segment) != 1 || sb.segment <= 0 )
	    {
		error(key, data, "illegal segment request");
	    }
	    else continue;
	}
	
	if (KEY("numbers"))
 	{
	    if (sscanf(data, "%d", &sb.numbers) != 1 || sb.numbers <= 0 )
	    {
		error(key, data, "illegal numbers request");
	    }
	    else continue;
	}
	
	if (KEY("fontsize"))
 	{
	    if (sscanf(data, "%d", &sb.fontsize) != 1 || sb.fontsize <= 0 )
	    {
		error(key, data, "illegal fontsize request");
	    }
	    else continue;
	}

	if (KEY("background"))
        {
            sb.bgcolor = yesno(key, data);
            continue;
        }

	if (KEY("width"))
 	{
 	sb.width = -1.;
 	ch = ' ';
 		if ((sscanf(data, "%lf%c", &sb.width, &ch)<1) || (sb.width < 0.))
 		{
 			sb.width = 1.;
 			error(key, data, "illegal grid width request");
 		}
 		if(ch=='i') sb.width = sb.width/72.0;
		continue;
	}

	error(key, data, "illegal request (scalebar)");
	
     }
     
     return 0;

}
