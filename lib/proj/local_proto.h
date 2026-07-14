/**
   \file lib/proj/local_proto.h

   \brief GProj library

   \author Paul Kelly <paul-grass stjohnspoint.co.uk>

   SPDX-FileCopyrightText: 2003-2008 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.
**/

struct ellps_list {
    char *name, *longname;
    double a, es, rf;
    struct ellps_list *next;
};

struct datum_list {
    char *name, *longname, *ellps;
    double dx, dy, dz;
    struct datum_list *next;
};

struct ellps_list *read_ellipsoid_table(int);
void free_ellps_list(struct ellps_list *);

struct datum_list *read_datum_table(void);
void free_datum_list(struct datum_list *);
