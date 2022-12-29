/* LIBDGL -- a Directed Graph Library implementation
 * Copyright (C) 2002 Roberto Micarelli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* best view tabstop=4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opt.h"

static int _ParseLongOption(GnoOption_s * pOpt, char *pszArg)
{
    char *pszLong;
    char *pszPar;
    char *pszMatch = NULL;
    int nret;


    if (pOpt->pszLong == NULL) {
	return 0;
    }

    pszLong = strdup(pOpt->pszLong);

    if ((pszPar = strchr(pszArg, '=')) != NULL) {
	*pszPar = 0;
    }
    pszMatch = strdup(pszArg);
    if (pszPar)
	*pszPar++ = '=';

    if (strcmp(pszLong, pszMatch + 2) == 0) {

	/* * mandatory parameter not found
	 * */
	if (pszPar == NULL) {
	    nret = -1;
	    goto free_and_exit;
	}

	if (pOpt->ppszValue) {
	    if (pOpt->ppszValue[0])
		free(pOpt->ppszValue[0]);
	    pOpt->ppszValue[0] = strdup(pszPar);
	}

	nret = 1;
	goto free_and_exit;
    }

    nret = 0;

  free_and_exit:

    free(pszLong);
    free(pszMatch);

    return nret;
}

static int _ParseLongSwitch(GnoOption_s * pOpt, char *pszArg)
{

    if (pOpt->pszLong == NULL) {
	return 0;
    }

    if (strcmp(pOpt->pszLong, pszArg + 2) == 0) {
	if (pOpt->pfValue)
	    *pOpt->pfValue = True;

	return 1;
    }

    return 0;
}


static int _ParseShortOption(GnoOption_s * pOpt, char *pszArg, char *pszPar)
{
    char *pszShort;
    int ich;

    if (pOpt->pszShort == NULL)
	return 0;

    pszShort = strdup(pOpt->pszShort);

    for (ich = 1; pszArg[ich]; ich++) {
	if (pszShort[0] == pszArg[ich]) {
	    if (pszPar == NULL || pszPar[0] == 0) {
		free(pszShort);

		return -1;
	    }

	    if (pszPar[0] == '-' && pszPar[1] != 0) {
		free(pszShort);

		return -1;
	    }

	    if (pOpt->ppszValue) {
		if (pOpt->ppszValue[0])
		    free(pOpt->ppszValue[0]);
		pOpt->ppszValue[0] = strdup(pszPar);
	    }

	    free(pszShort);

	    return 2;
	}
    }

    free(pszShort);

    return 0;
}

static int _ParseShortSwitch(GnoOption_s * pOpt, char *pszArg)
{
    int ich;

    if (pOpt->pszShort == NULL)
	return 0;

    for (ich = 1; pszArg[ich]; ich++) {
	if (pOpt->pszShort[0] == pszArg[ich]) {
	    if (pOpt->pfValue)
		*pOpt->pfValue = True;

	    return 1;
	}
    }

    return 0;
}

/***********************************************************************
 *				CALLBACKS
 **********************************************************************/

/***********************************************************************
 *				PUBLIC FUNCTIONS
 **********************************************************************/

/*@*--------------------------------------------------------------------
 * @func:       GnoParse()
 * @descr:      Parse argc, argv against the option array and setup option
 *                      values in the array.
 * 
 * @args:       I:      argc    =       count of argv entries
 *                      I:      argv    ->      array of pointer to string
 *                      I:      pOpt    ->      option array pointer
 *
 * @ret:        The number of 'orphan' entries found in the argv.
 * @see:        GnoOption_s
 *
 * @notes:      The argv array will be modified: each argv entry that contains a
 *                      recognized option ( '-.' or '--...' ) or each entry recognized as
 *                      a parametric option parameter, will be set to NULL.
 *                      Thus, at the function return the argv entries not set to NULL are
 *                      those of orphan entries (those not related to any option).
 *                      The user can then scan argv to find out orphans.
 *                      However the number of argv entries will not be altered.
 *
 *--------------------------------------------------------------------*/

int GnoParse(int argc, char **argv, GnoOption_s * pOpt)
{
    char *pszArgv;
    char *pszArgvNxt;
    int iArg, iOpt, cOrphan = 0;
    int nret, cret;
    Boolean fParseError = False;

    /* * this first loop setup default values
     * * strdup is used for non-switch options
     * * to make life easier when freeing the field
     * */
    for (iOpt = 0; pOpt[iOpt].pszShort || pOpt[iOpt].pszLong; iOpt++) {
	if (pOpt[iOpt].nFlg & GNO_FLG_SWITCH) {
	    if (pOpt[iOpt].pfValue) {
		pOpt[iOpt].pfValue[0] = pOpt[iOpt].fDef;
	    }
	}
	else {
	    if (pOpt[iOpt].pszDef) {
		if (pOpt[iOpt].ppszValue) {
		    pOpt[iOpt].ppszValue[0] = strdup(pOpt[iOpt].pszDef);
		}
	    }
	    else {
		if (pOpt[iOpt].ppszValue) {
		    pOpt[iOpt].ppszValue[0] = NULL;
		}
	    }
	}
    }

    /* * for each arg in argv lookup the matching options
     * */
    for (iArg = 0, pszArgv = NULL;
	 iArg < argc && (pszArgv = strdup(argv[iArg])) != NULL;
	 iArg++, free(pszArgv), pszArgv = NULL) {

	if (pszArgv[0] == '-' && pszArgv[1] == '-' && pszArgv[2]) {	/* long style */
	    for (iOpt = 0;
		 (pOpt[iOpt].pszShort || pOpt[iOpt].pszLong) && argv[iArg];
		 iOpt++) {
		if (pOpt[iOpt].pszLong) {
		    if (pOpt[iOpt].nFlg & GNO_FLG_SWITCH) {
			nret = _ParseLongSwitch(&pOpt[iOpt], pszArgv);
		    }
		    else {
			nret = _ParseLongOption(&pOpt[iOpt], pszArgv);
		    }

		    if (nret < 0) {
			fprintf(stderr,
				"parse option: syntax error at <%s>\n",
				pszArgv);
			fParseError = True;
		    }

		    if (nret == 1) {
			argv[iArg] = NULL;
		    }
		}
	    }

	    if (argv[iArg]) {
		fprintf(stderr, "parse option: <%s> is out of scope\n",
			pszArgv);
		fParseError = True;
	    }
	}
	else if (argv[iArg][0] == '-' && argv[iArg][1]) {	/* short style */
	    if (iArg + 1 < argc) {
		pszArgvNxt = strdup(argv[iArg + 1]);
	    }
	    else {
		pszArgvNxt = NULL;
	    }

	    for (cret = iOpt = 0;
		 pOpt[iOpt].pszShort || pOpt[iOpt].pszLong; iOpt++) {
		if (pOpt[iOpt].pszShort) {
		    if (pOpt[iOpt].nFlg & GNO_FLG_SWITCH) {
			nret = _ParseShortSwitch(&pOpt[iOpt], pszArgv);
		    }
		    else {
			nret =
			    _ParseShortOption(&pOpt[iOpt], pszArgv,
					      pszArgvNxt);
		    }
		    if (nret < 0) {
			fprintf(stderr,
				"parse option: syntax error at <%s>\n",
				pszArgv);
			fParseError = True;
		    }
		    else {
			cret = (nret > cret) ? nret : cret;
		    }
		}
	    }

	    if (pszArgvNxt) {
		free(pszArgvNxt);
	    }

	    if (cret == 1) {
		argv[iArg] = NULL;
	    }
	    else if (cret == 2) {
		argv[iArg++] = NULL;
		argv[iArg] = NULL;
	    }

	}
	else {
	    cOrphan++;
	}
    }

    if (pszArgv)
	free(pszArgv);

    return (fParseError == True) ? -1 : cOrphan;
}


/*@*--------------------------------------------------------------------
 * @func:       GnoFree()
 * @descr:      Free resource previously created with a call to GnoParse()
 * 
 * @args:       I:      pOpt    ->      option array pointer
 *
 * @see:        GnoOption_s, GnoParse()
 *
 *--------------------------------------------------------------------*/
void GnoFree(GnoOption_s * pOpt)
{
    int iOpt;

    for (iOpt = 0; pOpt[iOpt].pszShort || pOpt[iOpt].pszLong; iOpt++) {
	if (pOpt[iOpt].ppszValue) {
	    if (pOpt[iOpt].ppszValue[0]) {
		free(pOpt[iOpt].ppszValue[0]);
		pOpt[iOpt].ppszValue[0] = NULL;
	    }
	}
    }

}

/*@*--------------------------------------------------------------------
 * @func:       GnoHelp()
 * @descr:      Print a brief option's help on the standard error
 * 
 * @args:       I:      pszHead ->      help header string
 *
 * @args:       I:      pOpt    ->      option array pointer
 *
 * @see:        GnoOption_s
 *
 *--------------------------------------------------------------------*/
void GnoHelp(char *pszHead, GnoOption_s * pOpt)
{
    int iOpt;

    fprintf(stderr, "%s\n", (pszHead) ? pszHead : "options");

    for (iOpt = 0; pOpt[iOpt].pszShort || pOpt[iOpt].pszLong; iOpt++) {

	if (pOpt[iOpt].nFlg & GNO_FLG_SWITCH) {
	    if (pOpt[iOpt].pszShort) {
		fprintf(stderr, "-%s ", pOpt[iOpt].pszShort);
	    }

	    if (pOpt[iOpt].pszLong) {
		fprintf(stderr, "--%s", pOpt[iOpt].pszLong);
	    }

	    fprintf(stderr, "\n\t%s\n", (pOpt[iOpt].pszDescr)
		    ? pOpt[iOpt].pszDescr : "No description available.");
	}
	else {
	    if (pOpt[iOpt].pszShort) {
		fprintf(stderr, "-%s ", pOpt[iOpt].pszShort);

		fprintf(stderr, "<value> ");
	    }

	    if (pOpt[iOpt].pszLong) {
		fprintf(stderr, "--%s", pOpt[iOpt].pszLong);

		fprintf(stderr, "=<value>");
	    }

	    fprintf(stderr, "\n\t%s\n", (pOpt[iOpt].pszDescr)
		    ? pOpt[iOpt].pszDescr : "No description available.");
	}
    }

}

/******************************* END OF FILE **************************/
