
/**
   \file lib/proj/local_proto.h

   \brief GProj library

   \author Paul Kelly <paul-grass stjohnspoint.co.uk>

   (C) 2003-2008 by the GRASS Development Team
 
   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
**/

struct ellps_list
{
    char *name, *longname;
    double a, es, rf;
    struct ellps_list *next;
};

struct datum_list
{
    char *name, *longname, *ellps;
    double dx, dy, dz;
    struct datum_list *next;
};

struct ellps_list *read_ellipsoid_table(int);
void free_ellps_list(struct ellps_list *);

struct datum_list *read_datum_table(void);
void free_datum_list(struct datum_list *);
