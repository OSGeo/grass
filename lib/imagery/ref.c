
/****************************************************************************
 *
 * MODULE:       imagery library
 * AUTHOR(S):    Original author(s) name(s) unknown - written by CERL
 * PURPOSE:      Image processing library
 * COPYRIGHT:    (C) 1999, 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/***********************************************************
* I_fopen_group_ref_new (group)
* I_fopen_group_ref_old (group)
*
* fopen() the imagery group reference file (containing the number
* of files and the names of the cell files which comprise
* the group)
**********************************************************/
#include <grass/imagery.h>

FILE *I_fopen_group_ref_new(const char *group)
{
    return I_fopen_group_file_new(group, "REF");
}

FILE *I_fopen_group_ref_old(const char *group)
{
    return I_fopen_group_file_old(group, "REF");
}

FILE *I_fopen_group_ref_old2(const char *group, const char *mapset)
{
    return I_fopen_group_file_old2(group, mapset, "REF");
}

/*
   FILE *
   I_fopen_group_ref_append (
   const char *group)
   {
   return I_fopen_group_file_append (group, "REF");
   }
 */

FILE *I_fopen_subgroup_ref_new(const char *group, const char *subgroup)
{
    return I_fopen_subgroup_file_new(group, subgroup, "REF");
}

FILE *I_fopen_subgroup_ref_old(const char *group, const char *subgroup)
{
    FILE *fd;

    fd = I_fopen_subgroup_file_old(group, subgroup, "REF");
    return fd;
}

FILE *I_fopen_subgroup_ref_old2(const char *group, const char *subgroup, const char *mapset)
{
    FILE *fd;

    fd = I_fopen_subgroup_file_old2(group, subgroup, mapset, "REF");
    return fd;
}

