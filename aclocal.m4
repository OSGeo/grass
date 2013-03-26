
AC_DEFUN([LOC_CHECK_USE],[
AC_MSG_CHECKING(whether to use $2)
AC_MSG_RESULT("$with_$1")
case "$with_$1" in
	"no")	$3=	;;
	"yes")	$3="1"	;;
	*)	AC_MSG_ERROR([*** You must answer yes or no.])	;;
esac

])

AC_DEFUN([LOC_CHECK_INC_PATH],[
AC_MSG_CHECKING(for location of $2 includes)
case "$with_$1_includes" in
y | ye | yes | n | no)
	AC_MSG_ERROR([*** You must supply a directory to --with-$1-includes.])
	;;
esac
AC_MSG_RESULT($with_$1_includes)

if test -n "$with_$1_includes" ; then
    for dir in $with_$1_includes; do
        if test -d "$dir"; then
            $3="$$3 -I$dir"
        else
            AC_MSG_ERROR([*** $2 includes directory $dir does not exist.])
        fi
    done
fi
])

AC_DEFUN([LOC_CHECK_LIB_PATH],[
AC_MSG_CHECKING(for location of $2 library)
case "$with_$1_libs" in
y | ye | yes | n | no)
	AC_MSG_ERROR([*** You must supply a directory to --with-$1-libs.])
	;;
esac
AC_MSG_RESULT($with_$1_libs)

if test -n "$with_$1_libs"; then
    for dir in $with_$1_libs; do
        if test -d "$dir"; then
            $3="$$3 -L$dir"
        else
            AC_MSG_ERROR([*** $2 library directory $dir does not exist.])
        fi
    done
fi
])

AC_DEFUN([LOC_CHECK_FRAMEWORK_PATH],[
AC_MSG_CHECKING(for location of $2 framework)
case "$with_$1_framework" in
y | ye | yes | n | no)
	AC_MSG_ERROR([*** You must supply a directory to --with-$1-framework.])
	;;
esac
AC_MSG_RESULT($with_$1_framework)

if test -n "$with_$1_framework"; then
    if test -d $with_$1_framework; then
        $3="$$3 -F$with_$1_framework"
    else
        AC_MSG_ERROR([*** $2 framework directory $dir does not exist.])
    fi
fi
])

AC_DEFUN([LOC_CHECK_SHARE_PATH],[
AC_MSG_CHECKING(for location of $2 data files)
case "$with_$1_share" in
y | ye | yes | n | no)
        AC_MSG_ERROR([*** You must supply a directory to --with-$1-share.])
        ;;
esac
AC_MSG_RESULT($with_$1_share)

if test -n "$with_$1_share" ; then
    if test -d "$with_$1_share"; then
        $3="$with_$1_share"
    else
        AC_MSG_ERROR([*** $2 data directory $dir does not exist.])
    fi
fi
])

AC_DEFUN([LOC_CHECK_LDFLAGS],[
AC_MSG_CHECKING(for $2 linking flags)
case "$with_$1_ldflags" in
y | ye | yes | n | no)
	AC_MSG_ERROR([*** You must supply a directory to --with-$1-ldflags.])
	;;
esac
AC_MSG_RESULT($with_$1_ldflags)
$3="$$3 $with_$1_ldflags"
])

AC_DEFUN([LOC_CHECK_INCLUDES],[
ac_save_cppflags="$CPPFLAGS"
CPPFLAGS="$3 $CPPFLAGS"
AC_CHECK_HEADERS($1, [], ifelse($4,[],[
    AC_MSG_ERROR([*** Unable to locate $2 includes.])
], $4))
CPPFLAGS=$ac_save_cppflags
])

dnl $1  = library
dnl $2  = header
dnl $3  = function call
dnl $4  = descriptive name
dnl $5  = LDFLAGS initialiser
dnl $6  = result variable
dnl $7  = mandatory dependencies (not added to $5)
dnl $8  = mandatory dependencies (added to $5)
dnl $9  = ACTION-IF-NOT-FOUND

define(LOC_CHECK_LINK,[
ac_save_ldflags="$LDFLAGS"
ac_save_libs="$LIBS"
AC_MSG_CHECKING(for $4 library)
LDFLAGS="$5 $LDFLAGS"
LIBS="-l$1 $7 $8"
AC_TRY_LINK([$2],[$3],[
AC_MSG_RESULT(found)
$6="$$6 -l$1 $8"
],[
ifelse($9,[],[
    AC_MSG_ERROR([*** Unable to locate $4 library.])
],$9)
])
LIBS=${ac_save_libs}
LDFLAGS=${ac_save_ldflags}
])

dnl autoconf undefines "shift", so use "builtin([shift], ...)"

define(LOC_SHIFT1,[builtin([shift],$*)])
define(LOC_SHIFT2,[LOC_SHIFT1(LOC_SHIFT1($*))])
define(LOC_SHIFT4,[LOC_SHIFT2(LOC_SHIFT2($*))])
define(LOC_SHIFT8,[LOC_SHIFT4(LOC_SHIFT4($*))])
define(LOC_SHIFT9,[LOC_SHIFT1(LOC_SHIFT8($*))])

dnl $1  = library
dnl $2  = function
dnl $3  = descriptive name
dnl $4  = LDFLAGS initialiser
dnl $5  = result variable
dnl $6  = mandatory dependencies (not added to $5)
dnl $7  = mandatory dependencies (added to $5)
dnl $8  = ACTION-IF-NOT-FOUND
dnl $9+ = optional dependencies

define(LOC_CHECK_LIBS_0,[
AC_CHECK_LIB($1, $2, $5="$$5 -l$1 $7",[
[$8]
],$6 $7)
])

define(LOC_CHECK_LIBS_1,[
ifelse($9,[],
LOC_CHECK_LIBS_0($1,$2,,,$5,$6,$7,$8),
[
LOC_CHECK_LIBS_1($1,$2,,,$5,$6,$7,
LOC_CHECK_LIBS_1($1,$2,,,$5,$6,$7 $9,$8,LOC_SHIFT9($*)),
LOC_SHIFT9($*))
]
)
])

define(LOC_CHECK_LIBS,[
ac_save_ldflags="$LDFLAGS"
LDFLAGS="$4 $LDFLAGS"
LOC_CHECK_LIBS_1($1,$2,,,$5,$6,$7,
LDFLAGS=${ac_save_ldflags}
ifelse($8,[],[
    AC_MSG_ERROR([*** Unable to locate $3 library.])
],$8),LOC_SHIFT8($*))
LDFLAGS=${ac_save_ldflags}
])

dnl $1  = function
dnl $2  = descriptive name
dnl $3  = result variable
dnl $4  = LIBS initialiser (added to $3)
dnl $5  = LDFLAGS initialiser (not added to $3)
dnl $6  = LIBS initialiser (not added to $3)
dnl $7  = ACTION-IF-FOUND
dnl $8  = ACTION-IF-NOT-FOUND

define(LOC_CHECK_FUNC,[
ac_save_libs="$LIBS"
ac_save_ldflags="$LDFLAGS"
LIBS="$4 $6 $LIBS"
LDFLAGS="$5 $LDFLAGS"
AC_CHECK_FUNC($1,[
ifelse($7,[],[
    $3="$$3 $4"
],$7)
],[
ifelse($8,[],[
ifelse($2,[],
    [AC_MSG_ERROR([*** Unable to locate $1.])],
    [AC_MSG_ERROR([*** Unable to locate $2.])]
)
],$8)
])
LIBS=${ac_save_libs}
LDFLAGS=${ac_save_ldflags}
])

AC_DEFUN([LOC_CHECK_VERSION_STRING],[
AC_MSG_CHECKING($3 version)
ac_save_cppflags="$CPPFLAGS"
CPPFLAGS="$5 $CPPFLAGS"
AC_TRY_RUN([
#include <stdio.h> 
#include <$1>
int main(void) {
 FILE *fp = fopen("conftestdata","w");
 fputs($2, fp);
 return 0;
}
],
[   $4=`cat conftestdata`
    AC_MSG_RESULT($$4)],
[   AC_MSG_ERROR([*** Could not determine $3 version.]) ],
[   $4=$6
    AC_MSG_RESULT([unknown (cross-compiling)]) ])
CPPFLAGS=$ac_save_cppflags
])

AC_DEFUN([LOC_CHECK_SHARE],[
AC_CHECK_FILE($3/$1, [], ifelse($4,[],[
    AC_MSG_ERROR([*** Unable to locate $2 data files.])
], $4))
])

AC_DEFUN([LOC_CHECK_VERSION_INT],[
AC_MSG_CHECKING($3 version)
ac_save_cppflags="$CPPFLAGS"
CPPFLAGS="$5 $CPPFLAGS"
AC_TRY_RUN([
#include <stdio.h>
#include <$1>
int main(void) {
 FILE *fp = fopen("conftestdata","w");
 fprintf(fp, "%d", $2);
 return 0;
}
    ],
    [   $4=`cat conftestdata`
        AC_MSG_RESULT($$4)],
    [   AC_MSG_ERROR([*** Could not determine $3 version.]) ],
    [   $4=$6
        AC_MSG_RESULT([unknown (cross-compiling)]) ])
CPPFLAGS=$ac_save_cppflags
])

dnl autoconf undefines "eval", so use "builtin([eval], ...)"

AC_DEFUN([LOC_PAD],[$1[]ifelse(builtin([eval],len($1) > 23),1,[
                          ],substr([                        ],len($1)))])

AC_DEFUN([LOC_ARG_WITH],[
AC_ARG_WITH($1,
LOC_PAD([  --with-$1])[support $2 functionality (default: ]ifelse([$3],,yes,[$3])[)],,
[with_]patsubst([$1], -, _)[=]ifelse([$3],,yes,[$3]))
])

AC_DEFUN([LOC_ARG_WITH_INC],[
AC_ARG_WITH($1-includes,
LOC_PAD([  --with-$1-includes=DIRS])[$2 include files are in DIRS])
])

AC_DEFUN([LOC_ARG_WITH_LIB],[
AC_ARG_WITH($1-libs,
LOC_PAD([  --with-$1-libs=DIRS])[$2 library files are in DIRS])
])

AC_DEFUN([LOC_ARG_WITH_LDFLAGS],[
AC_ARG_WITH($1-ldflags,
LOC_PAD([  --with-$1-ldflags=FLAGS])[$2 needs FLAGS when linking])
])

AC_DEFUN([LOC_ARG_WITH_SHARE],[
AC_ARG_WITH($1-share,
LOC_PAD([  --with-$1-share=DIR])[$2 data files are in DIR])
])

AC_DEFUN([LOC_ARG_WITH_FRAMEWORK],[
AC_ARG_WITH($1-framework,
LOC_PAD([  --with-$1-framework=DIR])[$2 framework is in DIR])
])

AC_DEFUN([LOC_OPTIONAL],[
AC_MSG_CHECKING(whether to build $1)
if test -n "$USE_$2" ; then
	AC_MSG_RESULT(yes)
	BUILD_$3="$4"
else
	AC_MSG_RESULT(no)
	BUILD_$3=
fi
AC_SUBST(BUILD_$3)
])

dnl checks for complete floating-point support (infinity, NaN)

define(LOC_FP_TEST,[
#include <float.h>
int main(void) {
 double one = 1.0;
 double zero = 0.0;
 if (one/zero > DBL_MAX)        /* infinity */
   if (zero/zero != zero/zero)  /* NaN */
     return 0;
 return 1;
}
])

AC_DEFUN([LOC_CHECK_FP_INF_NAN],[
AC_MSG_CHECKING([for full floating-point support]$1)
AC_TRY_RUN(LOC_FP_TEST,
[   AC_MSG_RESULT(yes)
    $2],
[   AC_MSG_RESULT(no)
    $3],
[   AC_MSG_RESULT([unknown (cross-compiling)])
    $4]
)
])

dnl check whether the compiler supports the -mieee switch

AC_DEFUN([LOC_CHECK_CC_MIEEE],[
AC_MSG_CHECKING(whether "cc -mieee" works)
ac_save_cflags=${CFLAGS}
CFLAGS="$CFLAGS -mieee"
AC_TRY_COMPILE(,,
    [   AC_MSG_RESULT(yes)
        IEEEFLAG="-mieee"],
    [   AC_MSG_RESULT(no)])
CFLAGS=${ac_save_cflags}
])

AC_DEFUN([LOC_MSG],[
echo "$1"
])

AC_DEFUN([LOC_PAD_26],[substr([                           ],len($1))])

AC_DEFUN([LOC_YES_NO],[if test -n "${$1}" ; then echo yes ; else echo no ; fi])

AC_DEFUN([LOC_MSG_USE],[
[echo "  $1:]LOC_PAD_26($1)`LOC_YES_NO($2)`"])

AC_DEFUN(LOC_EXEEXT,
[AC_REQUIRE([AC_CYGWIN])
AC_REQUIRE([AC_MINGW32])
AC_MSG_CHECKING([for executable suffix])
AC_CACHE_VAL(ac_cv_exeext,
[if test "$CYGWIN" = yes || test "$MINGW32" = yes; then
  ac_cv_exeext=.exe
else
  ac_cv_exeext=no
fi])
EXEEXT=""
test x"${ac_cv_exeext}" != xno && EXEEXT=${ac_cv_exeext}
AC_MSG_RESULT(${ac_cv_exeext})
dnl Setting ac_exeext will implicitly change the ac_link command.
ac_exeext=$EXEEXT
AC_SUBST(EXEEXT)])

#------------------------------------------------------------------------
# SC_ENABLE_SHARED --
#
#	Allows the building of shared libraries
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-shared=yes|no
#
#	Defines the following vars:
#		STATIC_BUILD	Used for building import/export libraries
#				on Windows.
#
#	Sets the following vars:
#		SHARED_BUILD	Value of 1 or 0
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_SHARED], [
    AC_MSG_CHECKING([how to build libraries])
    AC_ARG_ENABLE(shared,
	[  --enable-shared         build and link with shared libraries [--enable-shared]],
	[shared_ok=$enableval], [shared_ok=yes])

    if test "${enable_shared+set}" = set; then
	enableval="$enable_shared"
	shared_ok=$enableval
    else
	shared_ok=yes
    fi

    if test "$shared_ok" = "yes" ; then
	AC_MSG_RESULT([shared])
	SHARED_BUILD=1
	GRASS_LIBRARY_TYPE='shlib'
    else
	AC_MSG_RESULT([static])
	SHARED_BUILD=0
	AC_DEFINE(STATIC_BUILD)
	GRASS_LIBRARY_TYPE='stlib'
    fi
    AC_SUBST(GRASS_LIBRARY_TYPE)
])

#--------------------------------------------------------------------
# SC_CONFIG_CFLAGS
#
#	Try to determine the proper flags to pass to the compiler
#	for building shared libraries and other such nonsense.
#
# Arguments:
#	none
#
# Results:
#
#	Defines and substitutes the following vars:
#
#       LDFLAGS -      Flags to pass to the compiler when linking object
#                       files into an executable application binary such
#                       as tclsh.
#       LD_SEARCH_FLAGS-Flags to pass to ld, such as "-R /usr/local/tcl/lib",
#                       that tell the run-time dynamic linker where to look
#                       for shared libraries such as libtcl.so.  Depends on
#                       the variable LIB_RUNTIME_DIR in the Makefile. Could
#                       be the same as CC_SEARCH_FLAGS if ${CC} is used to link.
#       CC_SEARCH_FLAGS-Flags to pass to ${CC}, such as "-Wl,-rpath,/usr/local/tcl/lib",
#                       that tell the run-time dynamic linker where to look
#                       for shared libraries such as libtcl.so.  Depends on
#                       the variable LIB_RUNTIME_DIR in the Makefile.
#       STLIB_LD -      Base command to use for combining object files
#                       into a static library.
#       SHLIB_CFLAGS -  Flags to pass to cc when compiling the components
#                       of a shared library (may request position-independent
#                       code, among other things).
#       SHLIB_LD -      Base command to use for combining object files
#                       into a shared library.
#       SHLIB_LD_FLAGS -Flags to pass when building a shared library. This
#                       differes from the SHLIB_CFLAGS as it is not used
#                       when building object files or executables.
#       SHLIB_LD_LIBS - Dependent libraries for the linker to scan when
#                       creating shared libraries.  This symbol typically
#                       goes at the end of the "ld" commands that build
#                       shared libraries. The value of the symbol is
#                       "${LIBS}" if all of the dependent libraries should
#                       be specified when creating a shared library.  If
#                       dependent libraries should not be specified (as on
#                       SunOS 4.x, where they cause the link to fail, or in
#                       general if Tcl and Tk aren't themselves shared
#                       libraries), then this symbol has an empty string
#                       as its value.
#       SHLIB_SUFFIX -  Suffix to use for the names of dynamically loadable
#                       extensions.  An empty string means we don't know how
#                       to use shared libraries on this platform.
#
#--------------------------------------------------------------------

AC_DEFUN([SC_CONFIG_CFLAGS], [
    SHLIB_CFLAGS=""
    SHLIB_LD_FLAGS=""
    SHLIB_SUFFIX=""
    SHLIB_LD=""
    STLIB_LD='${AR} cr'
    STLIB_SUFFIX='.a'
    GRASS_TRIM_DOTS='`echo ${LIB_VER} | tr -d .`'
    GRASS_LIB_VERSIONS_OK=ok
    LDFLAGS=""
    LD_SEARCH_FLAGS=""
    LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH"

    case $host in
        *-linux-*)
	    SHLIB_CFLAGS="-fPIC"
            SHLIB_LD_FLAGS=""
	    SHLIB_SUFFIX=".so"
	    SHLIB_LD="${CC} -shared"
            LDFLAGS="-Wl,--export-dynamic"
            LD_SEARCH_FLAGS='-Wl,-rpath-link,${LIB_RUNTIME_DIR}'
            LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH"
            ;;
        *-pc-cygwin)
            SHLIB_SUFFIX=".dll"
            SHLIB_LD="${CC} -shared"
            LDFLAGS="-Wl,--export-dynamic"
	    LD_LIBRARY_PATH_VAR="PATH"
	    ;;
        *-pc-mingw32 | *-pc-msys)
            SHLIB_SUFFIX=".dll"
            SHLIB_LD="${CC} -shared"
            LDFLAGS="-Wl,--export-dynamic,--enable-runtime-pseudo-reloc"
            LD_LIBRARY_PATH_VAR="PATH"
            ;;
	*-apple-darwin*)
	    SHLIB_CFLAGS="-fno-common"
	    SHLIB_SUFFIX=".dylib"
	    SHLIB_LD="cc -dynamiclib -compatibility_version \${GRASS_VERSION_MAJOR}.\${GRASS_VERSION_MINOR} -current_version \${GRASS_VERSION_MAJOR}.\${GRASS_VERSION_MINOR} -install_name \${INST_DIR}/lib/lib\${LIB_NAME}\${SHLIB_SUFFIX}"
	    LD_LIBRARY_PATH_VAR="DYLD_LIBRARY_PATH"
	    ;;
	*-sun-solaris*)
	    # Note: If _REENTRANT isn't defined, then Solaris
	    # won't define thread-safe library routines.
	    AC_DEFINE(_REENTRANT)
	    AC_DEFINE(_POSIX_PTHREAD_SEMANTICS)
	    # Note: need the LIBS below, otherwise Tk won't find Tcl's
	    # symbols when dynamically loaded into tclsh.
            if test "$GCC" = "yes" ; then
                SHLIB_CFLAGS="-fPIC"
                SHLIB_LD="$CC -shared"
                LD_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
            else
                SHLIB_CFLAGS="-KPIC"
                SHLIB_LD="/usr/ccs/bin/ld -G -z text"
                LD_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
            fi
            SHLIB_SUFFIX=".so"
            LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH"
	    ;;
	*-solaris2*)
	    # Note: Solaris is as of 2010 Oracle Solaris, not Sun Solaris
	    #       Oracle Solaris derives from Solaris 2 
	    #       derives from SunOS 5 
	    #       derives from UNIX System V Release 4
	    # Note: If _REENTRANT isn't defined, then Solaris
	    # won't define thread-safe library routines.
	    AC_DEFINE(_REENTRANT)
	    AC_DEFINE(_POSIX_PTHREAD_SEMANTICS)
	    # Note: need the LIBS below, otherwise Tk won't find Tcl's
	    # symbols when dynamically loaded into tclsh.
            if test "$GCC" = "yes" ; then
                SHLIB_CFLAGS="-fPIC"
                SHLIB_LD="$CC -shared"
                LD_SEARCH_FLAGS='-Wl,-R,${LIB_RUNTIME_DIR}'
            else
                SHLIB_CFLAGS="-KPIC"
                SHLIB_LD="/usr/ccs/bin/ld -G -z text"
                LD_SEARCH_FLAGS='-R ${LIB_RUNTIME_DIR}'
            fi
            SHLIB_SUFFIX=".so"
            LD_LIBRARY_PATH_VAR="LD_LIBRARY_PATH"
	    ;;
	*-freebsd*)
	    # NOTE: only FreeBSD 4+ is supported
	    # FreeBSD 3.* and greater have ELF.
	    SHLIB_CFLAGS="-fPIC"
	    #SHLIB_LD="ld -Bshareable -x"
	    SHLIB_LD="${CC} -shared"
	    SHLIB_SUFFIX=".so"
	    LDFLAGS="-export-dynamic"
	    #LD_SEARCH_FLAGS='-rpath ${LIB_RUNTIME_DIR}'
	    LD_SEARCH_FLAGS='-Wl,-rpath-link,${LIB_RUNTIME_DIR}'
	    # TODO: add optional pthread support with any combination of: 
	    # CFLAGS="$CFLAGS -pthread"
	    # LDFLAGS="$LDFLAGS -lpthread"
	    # AC_DEFINE(_REENTRANT)
	    # AC_DEFINE(_POSIX_PTHREAD_SEMANTICS)
	    ;;
	*-netbsd*)
	    # NetBSD has ELF.
	    SHLIB_CFLAGS="-fPIC"
	    SHLIB_LD="${CC} -shared"
	    SHLIB_LD_LIBS="${LIBS}"
	    LDFLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR} -export-dynamic'
	    SHLIB_LD_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR} -export-dynamic'
	    LD_SEARCH_FLAGS='-Wl,-rpath,${LIB_RUNTIME_DIR} -L${LIB_RUNTIME_DIR}'
	    # some older NetBSD versions do not handle version numbers with dots.
	    #STLIB_SUFFIX='${GRASS_TRIM_DOTS}.a'
	    #SHLIB_SUFFIX='${GRASS_TRIM_DOTS}.so'
	    #GRASS_LIB_VERSIONS_OK=nodots
	    # NetBSD 6 does handle version numbers with dots.
	    STLIB_SUFFIX=".a"
	    SHLIB_SUFFIX=".so"
	    # TODO: add optional pthread support with any combination of: 
	    # CFLAGS="$CFLAGS -pthread"
	    # LDFLAGS="$LDFLAGS -lpthread"
	    # AC_DEFINE(_REENTRANT)
	    # AC_DEFINE(_POSIX_PTHREAD_SEMANTICS)
	    ;;
	*aix*)
		# NOTE: do we need to support aix < 6 ?
	    LIBS="$LIBS -lc"
	    SHLIB_CFLAGS=""
	    SHLIB_SUFFIX=".so"
	    LDFLAGS=""
	    LD_SEARCH_FLAGS='-L${LIB_RUNTIME_DIR}'
	    LD_LIBRARY_PATH_VAR="LIBPATH"
	    GRASS_NEEDS_EXP_FILE=1
	    GRASS_EXPORT_FILE_SUFFIX='${LIB_VER}.exp'
	    ;;
        *)
            AC_MSG_ERROR([***Unknown platform: $host***])
            ;;
    esac

    AC_SUBST(LDFLAGS)
    AC_SUBST(LD_SEARCH_FLAGS)
    AC_SUBST(LD_LIBRARY_PATH_VAR)

    AC_SUBST(SHLIB_LD)
    AC_SUBST(SHLIB_LD_FLAGS)
    AC_SUBST(SHLIB_CFLAGS)
    AC_SUBST(SHLIB_SUFFIX)

    AC_SUBST(STLIB_LD)
    AC_SUBST(STLIB_SUFFIX)
])


dnl XXXX Begin Stolen from cdrtools-2.01 
dnl XXXX by Joerg Schilling <schilling fokus fraunhofer de> et al. XXXXXXXXX

dnl XXXXXXXXX Begin Stolen (but modified) from GNU tar XXXXXXXXXXXXXXXXXXXXX
dnl Changes:

dnl One line has been changed to:    [ac_save_CC="${CC-cc}" to default to "'cc"

dnl AC_SYS_LARGEFILE_MACRO_VALUE test moved from AC_FUNC_FSEEKO into AC_SYS_LARGEFILE
dnl Do not call AC_FUNC_FSEEKO because it does not check whether fseeko() is
dnl available on non Large File mode. There are additionoal tests for fseeko()/ftello()
dnl inside the AC_HAVE_LARGEFILES test.

dnl largefile_cc_opt definition added

#serial 18

dnl By default, many hosts won't let programs access large files;
dnl one must use special compiler options to get large-file access to work.
dnl For more details about this brain damage please see:
dnl http://www.sas.com/standards/large.file/x_open.20Mar96.html

dnl Written by Paul Eggert <eggert@twinsun.com>.

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_TEST_INCLUDES
AC_DEFUN([AC_SYS_LARGEFILE_TEST_INCLUDES],
  [[#include <sys/types.h>
    /* Check that off_t can represent 2**63 - 1 correctly.
       We can't simply "#define LARGE_OFF_T 9223372036854775807",
       since some C++ compilers masquerading as C compilers
       incorrectly reject 9223372036854775807.  */
#   define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
    int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721
			&& LARGE_OFF_T % 2147483647 == 1)
		       ? 1 : -1];
  ]])

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_MACRO_VALUE(C-MACRO, VALUE, CACHE-VAR, COMMENT, INCLUDES, FUNCTION-BODY)
AC_DEFUN([AC_SYS_LARGEFILE_MACRO_VALUE],
  [AC_CACHE_CHECK([for $1 value needed for large files], $3,
     [$3=no
      AC_TRY_COMPILE([$5],
	[$6], 
	,
	[AC_TRY_COMPILE([#define $1 $2]
[$5]
	   ,
	   [$6],
	   [$3=$2])])])
   if test "[$]$3" != no; then
     AC_DEFINE_UNQUOTED([$1], [$]$3, [$4])
   fi])

AC_DEFUN([AC_SYS_LARGEFILE],
  [AC_ARG_ENABLE(largefile,
     [  --enable-largefile      enable support for large files (LFS)])
   if test "$enable_largefile" = yes; then

     AC_CACHE_CHECK([for special C compiler options needed for large files],
       ac_cv_sys_largefile_CC,
       [ac_cv_sys_largefile_CC=no
        largefile_cc_opt=""
        if test "$GCC" != yes; then
	  # IRIX 6.2 and later do not support large files by default,
	  # so use the C compiler's -n32 option if that helps.
	  AC_TRY_COMPILE(AC_SYS_LARGEFILE_TEST_INCLUDES, , ,
	    [ac_save_CC="${CC-cc}"
	     CC="$CC -n32"
	     AC_TRY_COMPILE(AC_SYS_LARGEFILE_TEST_INCLUDES, ,
	       ac_cv_sys_largefile_CC=' -n32')
	     CC="$ac_save_CC"])
        fi])
     if test "$ac_cv_sys_largefile_CC" != no; then
       CC="$CC$ac_cv_sys_largefile_CC"
       largefile_cc_opt="$ac_cv_sys_largefile_CC"
     fi

     AC_SYS_LARGEFILE_MACRO_VALUE(_FILE_OFFSET_BITS, 64,
       ac_cv_sys_file_offset_bits,
       [Number of bits in a file offset, on hosts where this is settable.],
       AC_SYS_LARGEFILE_TEST_INCLUDES)
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGE_FILES, 1,
       ac_cv_sys_large_files,
       [Define for large files, on AIX-style hosts.],
       AC_SYS_LARGEFILE_TEST_INCLUDES)
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE, 1,
       ac_cv_sys_largefile_source,
       [Define to make fseeko visible on some hosts (e.g. glibc 2.2).],
       [#include <stdio.h>], [return !fseeko;])
   fi
  ])


AC_DEFUN([AC_FUNC_FSEEKO],
  [AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE, 1,
     ac_cv_sys_largefile_source,
     [Define to make fseeko visible on some hosts (e.g. glibc 2.2).],
     [#include <stdio.h>], [return !fseeko;])
   # We used to try defining _XOPEN_SOURCE=500 too, to work around a bug
   # in glibc 2.1.3, but that breaks too many other things.
   # If you want fseeko and ftello with glibc, upgrade to a fixed glibc.

   AC_CACHE_CHECK([for fseeko], ac_cv_func_fseeko,
     [ac_cv_func_fseeko=no
      AC_TRY_LINK([#include <stdio.h>],
        [return fseeko && fseeko (stdin, 0, 0);],
	[ac_cv_func_fseeko=yes])])
   if test $ac_cv_func_fseeko != no; then
     AC_DEFINE(HAVE_FSEEKO, 1,
       [Define if fseeko (and presumably ftello) exists and is declared.])
   fi])


dnl XXXXXXXXXXXXXXXXXX End Stolen (but modified) from GNU tar XXXXXXXXXXXXXX

AC_DEFUN([AC_HAVE_LARGEFILES],
[AC_CACHE_CHECK([if system supports Large Files at all], ac_cv_largefiles,
     	[AC_TRY_COMPILE([#include <stdio.h>
#include <sys/types.h>],
     		[
/*
 * Check that off_t can represent 2**63 - 1 correctly.
 * We can't simply "#define LARGE_OFF_T 9223372036854775807",
 * since some C++ compilers masquerading as C compilers
 * incorrectly reject 9223372036854775807.
 *
 * For MinGW, off64_t should be used and __MSVCRT_VERSION__ >= 0x0601
 * (msvcrt.dll version 6.10 or higher) is needed for _fstat64 and _stat64.
 */
#ifdef __MINGW32__
#   define LARGE_OFF_T (((off64_t) 1 << 62) - 1 + ((off64_t) 1 << 62))
#else
#   define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
#endif
    int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721
			&& LARGE_OFF_T % 2147483647 == 1)
		       ? 1 : -1];
#ifdef __MINGW32__
return !fseeko64;
return !ftello64;
#else
return !fseeko;
return !ftello;
#endif],
     		[ac_cv_largefiles=yes],
     		[ac_cv_largefiles=no])])
	if test $ac_cv_largefiles = yes; then
		AC_DEFINE(HAVE_LARGEFILES)
	fi])

dnl Checks for whether fseeko() is available in non large file mode
dnl and whether there is a prototype for fseeko()
dnl Defines HAVE_FSEEKO on success.
AC_DEFUN([AC_SMALL_FSEEKO],
[AC_CACHE_CHECK([for fseeko()], ac_cv_func_fseeko,
                [AC_TRY_LINK([#include <stdio.h>],
[return !fseeko;],
                [ac_cv_func_fseeko=yes],
                [ac_cv_func_fseeko=no])])
if test $ac_cv_func_fseeko = yes; then
  AC_DEFINE(HAVE_FSEEKO)
fi])

dnl Checks for whether ftello() is available in non large file mode
dnl and whether there is a prototype for ftello()
dnl Defines HAVE_FTELLO on success.
AC_DEFUN([AC_SMALL_FTELLO],
[AC_CACHE_CHECK([for ftello()], ac_cv_func_ftello,
                [AC_TRY_LINK([#include <stdio.h>],
[return !ftello;],
                [ac_cv_func_ftello=yes],
                [ac_cv_func_ftello=no])])
if test $ac_cv_func_ftello = yes; then
  AC_DEFINE(HAVE_FTELLO)
fi])

dnl XXXXXXXXXXX End Stolen from cdrtools-2.01 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

