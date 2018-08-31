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

/*@*********************************************************************
 * @pack:       GNO_PACK
 * @descr:      Command line options utility.
 *
 * @notes:      Support to easily parse command line options. Some concepts
 *                      were taken from the posix getopt() family functions, while
 *                      our specific experience in C programming suggested us a
 *                      proprietary implementation to make source code more readable
 *                      at life easier (I hope).
 *                      Option format:
 *
 *                      syntax                          name
 *                      ----------------------------------------------
 *                      --option=value          long-parametric
 *                      --option                        long-boolean
 *                      -option value           short-parametric
 *                      -option                         short-boolean
 *
 *              C sample:
 *              ----------------------------------------------
 *              #include <stdio.h>
 *              #include <opt.h>
 *
 *              int main( int argc , char ** argv )
 *              {
 *                      Boolean         fHelp;
 *                      Boolean         fTcp;
 *                      Boolean         fUdp;
 *                      char    *       pszProtocol;
 *                      char    *       pszInterface;
 *                      int                     i;
 *
 *                      GNO_BEGIN
 *                      GNO_SWITCH( "h", "help", False , & fHelp , "Print this help." )
 *                      GNO_SWITCH( "t", "tcp", False , & fTcp , NULL )
 *                      GNO_SWITCH( "u", "udp", False , & fUdp , NULL )
 *                      GNO_OPTION( "p", "protocol", "tcp" , & pszProtocol , NULL )
 *                      GNO_OPTION( "i", "interface", "eth0" , & pszInterface , NULL )
 *                      GNO_END
 *
 *
 *                      if ( GNO_PARSE( argc , argv ) < 0 )
 *                      {
 *                              return 1;
 *                      }
 *
 *                      if ( fHelp == True )
 *                      {
 *                              GNO_HELP( "t_opt usage" );
 *                              return 0;
 *                      }
 *
 *                      printf ( "t/tcp  = %s\n", (fTcp  == True) ? "True" : "False" );
 *                      printf ( "u/udp  = %s\n", (fUdp  == True) ? "True" : "False" );
 *                      printf ( "p/protocol  = <%s>\n", pszProtocol );
 *                      printf ( "i/interface = <%s>\n", pszInterface );
 *
 *                      GNO_FREE();
 *
 *                      printf( "orphan options:\n" );
 *
 *                      for ( i = 0 ; i < argc ; i ++ )
 *                      {
 *                              if ( argv[ i ] )
 *                              {
 *                                      printf( "arg %d: %s\n", i, argv[i ] );
 *                              }
 *                              else
 *                              {
 *                                      printf( "arg %d: --\n", i );
 *                              }
 *                      }
 *                      return 0;
 *              }
 *                                              
 **********************************************************************/

#ifndef _GNOPT_H_
#define _GNOPT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************************
 *				DEFINES
 **********************************************************************/
    /*@*--------------------------------------------------------------------
     * @defs:       GNO_FLG
     * @descr:      flags used to set the nFlg field in the GnoOption_s
     *                      SWITCH    = boolean option required
     *
     * @see:        GnoOption_s
     *
     *--------------------------------------------------------------------*/

#define GNO_FLG_SWITCH		0x01

    /*@*--------------------------------------------------------------------
     * @defs:       True/False
     * @descr:      one more useful (?) true/false definition
     *
     *--------------------------------------------------------------------*/
#define True  1
#define False 0

/***********************************************************************
 *				STRUCTS/UNIONS/TYPES/FUNCDEFS
 **********************************************************************/
    /*@*--------------------------------------------------------------------
     * @type:       Boolean
     * @descr:      wasn't it better to use 'int'?
     *--------------------------------------------------------------------*/
    typedef int Boolean;

    /*@*--------------------------------------------------------------------
     * @type:       GnoOption_s
     * @descr:      This structure describes an option. We intend to use an array
     *          of GnoOption_s, terminated by a 'NULL' entry (one containing
     *                      all binary-zero fields), and to pass it to GnoParse() together
     *                      with the typical argc,argv couple of main() args. The array's
     *                      entries are looked-up and filled with coherent argc,argv
     *                      values. Some fields are statically filled by user while other
     *                      are returned by the GnoParse() function.
     *                      Fields set by the user:
     *
     *                      nFlg      = So far the only supported flag is GNO_FLG_SWITCH
     *                                  to address a boolean option type.
     *                      fDef      = The default value for a boolean option.
     *                      pszDef    = The default value for a parametric option.
     *                      pszShort  = The short name of the option.
     *                      pszLong   = The long name of the option.
     *                      pfValue   = Pointer to a boolean option return value.
     *                      ppszValue = Pointer to a parametric option return value.
     *                      pszDescr  = A brief option description
     *
     *                      Fields set by GnoParse():
     *
     *                      iArg       = argv option index
     *                      *ppszValue = pointer to parametric option value 
     *                      *pfValue   = True/False
     *
     *                      User supplied fields are mandatory only within specific
     *                      conditions:
     *
     *                      - at least one of pszShort/pszLong must be specified
     *                      - fDef and pfValue apply to a boolean option only
     *                      - pszDef and ppszValue apply to a parametric option only
     *                      - pszDescr is optional
     *
     * @see:        GnoParse(), GNO_FLG
     *
     *--------------------------------------------------------------------*/

    typedef struct GnoOption
    {
	int iArg;		/* returned argv option index */
	int nFlg;		/* flags describing the option */
	Boolean fDef;		/* default returned value for a boolean option */
	char *pszDef;		/* default returned value for a parametric option */
	char *pszShort;		/* short-option recogniser */
	char *pszLong;		/* long-option recogniser */
	Boolean *pfValue;	/* address to return a boolean option */
	char **ppszValue;	/* address to return a parametric option */
	char *pszDescr;		/* a brief option description */

    } GnoOption_s;


/***********************************************************************
 *				MACROS
 **********************************************************************/
    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_BEGIN
     * @descr:      Begin an option array declaration
     * 
     * @notes:      The best use is to start option declaration immediately after
     *                      the automatic-variable declaration in the main() function.
     *
     *--------------------------------------------------------------------*/
#define GNO_BEGIN GnoOption_s _aopt[] = {

    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_OPTION
     * @descr:      Declare a parametric option
     *
     * 
     * @args:       I:      chsopt  =       short option character
     *                      I:      pszlopt ->      long option psz
     *                      I:      pszdef  ->      default-returned psz
     *                      I:      ppszv   ->      user-addressed return variable pointer
     *
     *--------------------------------------------------------------------*/
#define GNO_OPTION(pszsopt,pszlopt,pszdef,ppszv,pszdescr) \
			{ 0, 0, 0, pszdef, pszsopt, pszlopt, NULL, ppszv, pszdescr },

    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_SWITCH
     * @descr:      Declare a boolean option
     * 
     * @args:       I:      chsopt  =       short option character
     *                      I:      pszlopt ->      long option psz
     *                      I:      fdef    =       default-returned Boolean
     *                      I:      pfv             ->      user-addressed return variable pointer
     *
     *--------------------------------------------------------------------*/
#define GNO_SWITCH(pszsopt,pszlopt,fdef,pfv,pszdescr) \
			{	\
				0, \
				GNO_FLG_SWITCH, \
				fdef, NULL, \
				pszsopt, pszlopt, \
				pfv, NULL, \
				pszdescr \
			},

    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_PARSE
     * @descr:      Parse a GnoOption_s array declaration
     * 
     * @args:       I:      argc    =       count of argv entries
     *                      I:      argv    ->      array of sz pointers
     *
     *--------------------------------------------------------------------*/
#define GNO_PARSE(argc,argv) GnoParse( (argc), (argv), _aopt )

    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_END
     * @descr:      Terminate an option array declaration
     * 
     *--------------------------------------------------------------------*/
#define GNO_END	{ 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL } };

    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_HELP
     *
     * @descr:      Print a brief option's help on the standard error
     *
     * @args:       I:      pszhead ->      help header string
     * 
     *--------------------------------------------------------------------*/
#define GNO_HELP(pszhead) GnoHelp( pszhead , _aopt )

    /*@*--------------------------------------------------------------------
     *
     * @macro:      GNO_FREE
     *
     * @descr:      Free resources created by a previously parsed array
     *
     * 
     *--------------------------------------------------------------------*/
#define GNO_FREE() GnoFree( _aopt )


/***********************************************************************
 *				FUNCTION PROTOTYPES
 **********************************************************************/

    extern int GnoParse(int argc, char **argv, GnoOption_s * pOpt);

    extern void GnoFree(GnoOption_s * pOpt);

    extern void GnoHelp(char *pszHead, GnoOption_s * pOpt);


#ifdef __cplusplus
}
#endif

#endif				/* top of file */

/***************************** END OF FILE ****************************/
