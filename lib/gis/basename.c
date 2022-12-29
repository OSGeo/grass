
/*!
 * \file lib/gis/basename.c
 *
 * \brief GIS Library - Program basename routines.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 */

#include <grass/gis.h>

#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


/*!
 * \brief Truncates filename to the base part (before the last '.')
 * if it matches the extension, otherwise leaves it unchanged.
 * 
 * Checks if a filename matches a certain file extension
 * (case insensitive) and if so, truncates the string to the
 * base file name (cf. basename Unix command)
 *
 * \param filename string containing filename
 * \param desired_ext string containing extension to look for (case
 * insensitive)
 *
 * \return pointer to filename
 */
char *G_basename(char *filename, const char *desired_ext)
{
    /* Find the last . in the filename */
    char *dot = strrchr(filename, '.');

    if (dot && G_strcasecmp(dot + 1, desired_ext) == 0)
	*dot = '\0';

    return filename;
}



/*!
 * \brief Get number of decimals from a string
 *
 * \param str String to analyse
 * 
 * \return number of decimals
 */
size_t G_get_num_decimals(const char *str)
{
    int sep = '.';
    size_t len;
    char *sep_ptr = strchr(str, sep);
    if (sep_ptr == NULL)
        return 0;
    len = strlen(str);
    return len - (size_t)(sep_ptr - str) - 1;
}

/*!
 * \brief Convert a double to a string substituting the dot with underscore
 *        12.3456 => '12_3456'
 *
 * \param number the double number that will be convert to string
 * \param ndigits the number of integer digits in the output string
 * \param ndecimals the number of decimals in the output string
 *
 * \return a formatted string
 */
char *G_double_to_basename_format(double number, 
                                  size_t ndigits, size_t ndecimals)
{
    double integer, decimal;
    integer = floor(number);
    char intfmt[GNAME_MAX] = "%d";
    char intstr[GNAME_MAX];
    char decfmt[GNAME_MAX] = "";
    char decstr[GNAME_MAX] = "";
    char *result;

    if (ndigits != 0){
        sprintf(intfmt, "%%0%zud", ndigits);
    }
    sprintf(intstr, intfmt, (int)integer);

    if (ndecimals != 0){
        sprintf(decfmt, "_%%0%zud", ndecimals);
        decimal = ((number - integer) * pow(10., (double)ndecimals));
        sprintf(decstr, decfmt, (int)decimal);
    }
    result = G_malloc(strlen(intstr) + strlen(decstr) + 1);
    sprintf(result, "%s%s", intstr, decstr);
    return result;
}


/*!
 * \brief Return the environmental basename variable or the default
 * value
 *
 * return pointer to basename separator
 */
char *G_get_basename_separator()
{
    char *envvar = "GRASS_BASENAME_SEPARATOR";
    char *envsep;

    envsep = getenv(envvar);
    return (envsep != NULL && strlen(envsep) > 0) ? envsep: GBASENAME_SEP;
}


/*!
 * \brief join an array of strings using the basename separator
 * 
 * \param strings is an array of strings
 * \param len is the length of the array
 * 
 * \return a joined string
 */
char *G_join_basename_strings(const char**strings, size_t len)
{
    size_t i, length, lensep;
    char *result;
    char *separator;

    separator = G_get_basename_separator();

    lensep = strlen(separator);
    length = lensep * (len - 1) + 1;
    for (i = 0; i < len; i++){
        length += strlen(strings[i]);
    }
    result = G_malloc(length);

    if (result)
    {
        strcpy(result, strings[0]);
        for (i = 1; i < len; i++){
            strcat(result, separator);
            strcat(result, strings[i]);
        }
    }

    return result;
}


/*!
 * \brief Generate the format string
 *
 * \param basename String with the basename
 * \param digits Number of digits number
 * \param decimals Number of decimal number
 * \param filler String used to fill, default is 0
 * 
 * \return Format string
 */
char *G_generate_basename(const char *basename, double number, 
                         size_t ndigits, size_t ndecimals)
{
    char *separator, *numberstr, *result;

    separator = G_get_basename_separator();
    numberstr = G_double_to_basename_format(number, ndigits, ndecimals);

    result = G_malloc(strlen(basename) + strlen(separator) + strlen(numberstr) + 1);

    if (result)
        sprintf(result, "%s%s%s", basename, separator, numberstr);
    return result;
}


