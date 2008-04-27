/****************************************************************************
 *
 * MODULE:       r.what
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,Brad Douglas <rez touchofmadness.com>,
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_nospam yahoo.com>, Soeren Gebbert <soeren.gebbert gmx.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define NFILES 150

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"
#include <grass/glocale.h>

struct order {
    int point;
    int row;
    int col;
    char north_buf[256];
    char east_buf[256];
    char lab_buf[256];
    char clr_buf[NFILES][256];
    CELL value[NFILES];
    DCELL dvalue[NFILES];
};

static int by_row(const void *, const void *);
static int by_point(const void *, const void *);

static int tty = 0;

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int main(int argc,char *argv[])
{
  char *mapset;
  int i, j;
  int nfiles;
  int withcats;
  int fd[NFILES];
  struct Categories cats[NFILES];
  struct Cell_head window;
  struct Colors ncolor[NFILES];
  struct Colors colors;
  RASTER_MAP_TYPE out_type[NFILES];
  CELL *cell[NFILES];
  DCELL *dcell[NFILES];
/*   int row, col; */
  double drow, dcol;
  int row_in_window, in_window;
  double east, north;
  int line;
  char buffer[1024];
  char **ptr;
  struct Option *opt1, *opt2, *opt3, *opt4, *opt_fs;
  struct Flag *flag1, *flag2, *flag3, *flag4;
  char fs;
  int Cache_size;
  int done = 0;
  int point, point_cnt;
  struct order *cache;
  int cur_row;
  int projection;
  int cache_hit= 0, cache_miss= 0;
  int cache_hit_tot= 0, cache_miss_tot= 0;
  int pass = 0;
  int cache_report = 0;
  char tmp_buf[500], *null_str;
  int red, green, blue;
  struct GModule *module;
  

  G_gisinit (argv[0]);
  
  /* Set description */
  module              = G_define_module();
  module->keywords = _("raster");
    module->description = 
  _("Queries raster map layers on their category values and category labels.");

  opt1 = G_define_option() ;
  opt1->key        = "input" ;
  opt1->type       = TYPE_STRING ;
  opt1->required   = YES ;
  opt1->multiple   = YES ;
  opt1->gisprompt  = "old,cell,raster" ;
  opt1->description= _("Name of existing raster map(s) to query");

  opt2 = G_define_option() ;
  opt2->key        = "cache" ;
  opt2->type       = TYPE_INTEGER ;
  opt2->required   = NO ;
  opt2->multiple   = NO ;
  opt2->description= _("Size of point cache") ;
  opt2->answer     = "500";

  opt3 = G_define_option() ;
  opt3->key        = "null";
  opt3->type       = TYPE_STRING;
  opt3->required   = NO;
  opt3->answer     = "*";
  opt3->description= _("Char string to represent no data cell") ;

  opt_fs = G_define_standard_option(G_OPT_F_SEP);

  opt4 = G_define_option() ;
  opt4->key        = "east_north";
  opt4->type       = TYPE_DOUBLE;
  opt4->key_desc   = "east,north";
  opt4->required   = NO;
  opt4->multiple   = YES;
  opt4->description= _("Coordinates for query");

  flag1 = G_define_flag() ;
  flag1->key         = 'f' ;
  flag1->description = _("Show the category label in the grid cell(s)") ;

  flag2 = G_define_flag() ;
  flag2->key         = 'c' ;
  flag2->description = _("Turn on cache reporting") ;

  flag3 = G_define_flag();
  flag3->key = 'i';
  flag3->description = _("Output integer category values, not cell values");

  flag4 = G_define_flag();
  flag4->key = 'r';
  flag4->description = _("Output color values as RRR:GGG:BBB");

  if (G_parser(argc, argv))
      exit(EXIT_FAILURE);

  tty = isatty (0);

  projection = G_projection();

    /* see v.in.ascii for a better solution */
    if (opt_fs->answer != NULL) {
        if (strcmp(opt_fs->answer, "space") == 0)
            fs = ' ';
        else if (strcmp(opt_fs->answer, "tab") == 0)
            fs = '\t';
        else if (strcmp(opt_fs->answer, "\\t") == 0)
            fs = '\t';
        else
            fs = opt_fs->answer[0];
    }

  withcats = 0;
  nfiles = 0;

  if (tty)
      Cache_size = 1;
  else
    Cache_size = atoi (opt2->answer);

  null_str = opt3->answer;

  if (Cache_size < 1) Cache_size = 1;

  cache = (struct order *) G_malloc (sizeof (struct order) * Cache_size);

  /* note this is not kosher */
  withcats = flag1->answer;

  /*enable cache report*/
  if(flag2->answer)
  cache_report = 1;

  ptr = opt1->answers;
  for (; *ptr != NULL; ptr++)
  { 
      char name[GNAME_MAX];

      if (nfiles >= NFILES)
	  G_fatal_error (_("%s: can only do up to %d raster maps, sorry\n"),
	      G_program_name(), NFILES);

      strcpy (name, *ptr);
      if(NULL == (mapset = G_find_cell2 (name, "")))
	  die (name, " - not found");
      if(0 > (fd[nfiles] = G_open_cell_old (name, mapset)))
	  die ("can't open", name);

      out_type[nfiles] = G_get_raster_map_type(fd[nfiles]);
      if(flag3->answer) out_type[nfiles] = CELL_TYPE;

      if(flag4->answer)
      {
              G_read_colors(name, mapset, &colors);
              ncolor[nfiles] = colors;
      }

      if (withcats && G_read_cats (name, mapset, &cats[nfiles]) < 0)
  	     die (name, " - can't read category file");
      nfiles++;
  }

  for (i = 0 ; i < nfiles ; i++)
  {
    if(flag3->answer)
	 out_type[i] = CELL_TYPE;

    cell[i] = G_allocate_c_raster_buf();
    if(out_type[i] != CELL_TYPE)
        dcell[i] = G_allocate_d_raster_buf();
  }

  G_get_window (&window);

  line = 0;
  if (!opt4->answers && tty)
      fprintf (stderr, "enter points, \"end\" to quit\n");

  j = 0;
  done = 0;
  while (!done)
  {
    pass++;
    if (cache_report & !tty)
	fprintf (stderr, "Pass %3d  Line %6d   - ", pass, line);

    cache_hit = cache_miss = 0;
    if (!opt4->answers && tty)
    {
      fprintf (stderr, "\neast north [label] >  ");
      Cache_size = 1;
    }
    {
      point_cnt = 0;
      for (i = 0 ; i < Cache_size ; i++)
      {
	if (!opt4->answers && fgets(buffer,1000,stdin) == NULL)
	  done = 1;
	else
	{
	  line++;
	  if ((!opt4->answers &&
	      (strncmp (buffer, "end\n",  4) == 0 ||
	       strncmp (buffer, "exit\n", 5) == 0)) ||
	      (opt4->answers && !opt4->answers[j]))
	    done = 1;
	  else
	  {
	    *(cache[point_cnt].lab_buf) = *(cache[point_cnt].east_buf) = *(cache[point_cnt].north_buf) = 0;
	    if(!opt4->answers)
	        sscanf (buffer, "%s %s %[^\n]", cache[point_cnt].east_buf, cache[point_cnt].north_buf, cache[point_cnt].lab_buf);
	    else
	    {
		strcpy(cache[point_cnt].east_buf, opt4->answers[j++]);
		strcpy(cache[point_cnt].north_buf, opt4->answers[j++]);
	    }
	    if (*(cache[point_cnt].east_buf) == 0)
	      continue;        /* skip blank lines */

	    if (*(cache[point_cnt].north_buf) == 0)
	    {
	      oops (line, buffer, "two coordinates (east north) required");
	      continue;
	    }
	    if (!G_scan_northing (cache[point_cnt].north_buf, &north, window.proj)
	      ||  !G_scan_easting (cache[point_cnt].east_buf, &east, window.proj))
	    {
	      oops (line, buffer, "invalid coordinate(s)");
	      continue;
	    }

	    /* convert north, east to row and col */
	    drow = G_northing_to_row (north, &window);
	    dcol = G_easting_to_col (east, &window);

	    /* a special case.
	    *   if north falls at southern edge, or east falls on eastern edge,
	    *   the point will appear outside the window.
	    *   So, for these edges, bring the point inside the window
	    */
	    if (drow == window.rows) drow--;
	    if (dcol == window.cols) dcol--;

	    cache[point_cnt].row = (int) drow;
	    cache[point_cnt].col = (int) dcol;
	    cache[point_cnt].point = point_cnt;
	    point_cnt++;
	  }
	}
      }
    }

    if (Cache_size > 1)
      qsort (cache, point_cnt, sizeof (struct order), by_row);

    /* extract data from files and store in cache */

    cur_row = -99;

    for (point = 0 ; point < point_cnt ; point++)
    {
      row_in_window = 1;
      in_window = 1;
      if (cache[point].row < 0 || cache[point].row >= window.rows)
	row_in_window = in_window = 0; 
      if (cache[point].col < 0 || cache[point].col >= window.cols)
	in_window = 0; 

      if (!in_window)
      {
	if (tty)
	  fprintf (stderr,
	    "** note ** %s %s is outside your current window\n",
	    cache[point].east_buf, cache[point].north_buf);
      }

      if (cur_row != cache[point].row)
      {
	  cache_miss++;
	  if (row_in_window)
	    for (i = 0; i < nfiles; i++)
	    {
	      if (G_get_c_raster_row (fd[i], cell[i], cache[point].row) < 0)
	          die (argv[i+1], " - can't read");
               
	      if(out_type[i] != CELL_TYPE)
	      {
	        if (G_get_d_raster_row (fd[i], dcell[i], cache[point].row) < 0)
		    die (argv[i+1], " - can't read");
              }
            }	

	  cur_row = cache[point].row;
      }
      else
	cache_hit++;

      for (i = 0; i < nfiles; i++)
      {
	 if(in_window)
	    cache[point].value[i] = cell[i][cache[point].col];
         else
	    G_set_c_null_value(&(cache[point].value[i]), 1);

	 if(out_type[i] != CELL_TYPE)
         {
	    if(in_window)
	       cache[point].dvalue[i] = dcell[i][cache[point].col];
            else
	       G_set_d_null_value(&(cache[point].dvalue[i]), 1);
         }
        if (flag4->answer)
        {
                if (out_type[i] == CELL_TYPE)
                        G_get_c_raster_color(&cell[i][cache[point].col],
                                &red, &green, &blue, &ncolor[i]);
                else
                        G_get_d_raster_color(&dcell[i][cache[point].col],
                                &red, &green, &blue, &ncolor[i]);

                sprintf(cache[point].clr_buf[i], "%03d:%03d:%03d", red, green, blue);
        }

      }
    } /* point loop */

    if (Cache_size > 1)
      qsort (cache, point_cnt, sizeof (struct order), by_point);

    /* report data from re-ordered cache */

    for (point = 0 ; point < point_cnt ; point++)
    {

    G_debug(1, "%s|%s at col %d, row %d\n",
      cache[point].east_buf, cache[point].north_buf, cache[point].col, cache[point].row);


      fprintf (stdout,"%s%c%s%c%s", cache[point].east_buf, fs, cache[point].north_buf, 
		    fs, cache[point].lab_buf);

      for (i = 0; i < nfiles; i++)
      {
	 if(out_type[i] == CELL_TYPE)
         {
	    if(G_is_c_null_value(&cache[point].value[i]))
	    {
	       fprintf (stdout,"%c%s", fs, null_str);
               continue;
            }
	    fprintf (stdout,"%c%ld", fs, (long) cache[point].value[i]);
         }
	 else /* FCELL or DCELL */
         {
	    if(G_is_d_null_value(&cache[point].dvalue[i]))
	    {
	       fprintf (stdout,"%c%s", fs, null_str);
               continue;
            }
	    sprintf (tmp_buf,"%.10f", cache[point].dvalue[i]);
	    G_trim_decimal(tmp_buf);
	    fprintf (stdout,"%c%s", fs, tmp_buf);
         }
	 if (withcats)
	    fprintf (stdout,"%c%s", fs, G_get_cat (cache[point].value[i], &cats[i]));
	 if (flag4->answer)
	    fprintf (stdout,"%c%s", fs, cache[point].clr_buf[i]);
      }
      fprintf (stdout,"\n");
    }

    if (cache_report & !tty)
	fprintf (stderr, "Cache  Hit: %6d  Miss: %6d\n", 
	    cache_hit, cache_miss);

    cache_hit_tot += cache_hit;
    cache_miss_tot += cache_miss;
    cache_hit = cache_miss = 0;
  }
    if (cache_report & !tty)
	fprintf (stderr, "Total:    Cache  Hit: %6d  Miss: %6d\n", 
	    cache_hit_tot, cache_miss_tot);
  exit(EXIT_SUCCESS);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int 
oops (int line, char *buf, char *msg)
{
    static int first = 1;
    if (!tty)
    {
	if (first)
	{
	    G_warning("%s: ** input errors **\n",
		G_program_name());
	    first = 0;
	}
	G_warning("line %d: %s\n", line, buf);
    }
    G_warning ("** %s **\n", msg);

    return 0;
}


/* *************************************************************** */
/* for qsort,  order list by row ********************************* */
/* *************************************************************** */


static int by_row (const void *ii, const void *jj)
{
  const struct order *i = ii, *j = jj;
  return i->row - j->row;
}


/* *************************************************************** */
/* for qsort,  order list by point ******************************* */
/* *************************************************************** */

/* for qsort,  order list by point */
static int by_point (const void *ii, const void *jj)
{
  const struct order *i = ii, *j = jj;
  return i->point - j->point;
}
