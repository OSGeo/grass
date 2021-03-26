/* include/config.h.  Generated automatically by configure.  */

/*
 * config.h.in
 */

#ifndef _config_h
#define _config_h

#define GDEBUG 1

/* define _OE_SOCKETS flag (OS/390 sys/socket.h) */
/* #undef _OE_SOCKETS */

/* define _REENTRANT flag (for SunOS) */
/* #undef _REENTRANT */

/* define USE_DELTA_FOR_TZ (for AIX) */
/* #undef USE_DELTA_FOR_TZ */

/* define for Windows static build */
/* #undef STATIC_BUILD */

/* define if limits.h exists */
#define HAVE_LIMITS_H 1

/* define if termio.h exists */
#define HAVE_TERMIO_H 1

/* define if termios.h exists */
#define HAVE_TERMIOS_H 1

/* define if unistd.h exists */
#define HAVE_UNISTD_H 1

/* define if values.h exists */
#define HAVE_VALUES_H 1

/* define if zlib.h exists */
#define HAVE_ZLIB_H 1

/* define if bzlib.h exists */
#define HAVE_BZLIB_H 1

/* define if zstd.h exists */
#define HAVE_ZSTD_H 1

/* define if sys/ioctl.h exists */
#define HAVE_SYS_IOCTL_H 1

/* define if sys/mtio.h exists */
#define HAVE_SYS_MTIO_H 1

/* define if sys/resource.h exists */
#define HAVE_SYS_RESOURCE_H 1

/* define if sys/time.h exists */
#define HAVE_SYS_TIME_H 1

/* define if time.h and sys/time.h can be included together */
#define TIME_WITH_SYS_TIME 1

/* define if sys/timeb.h exists */
#define HAVE_SYS_TIMEB_H 1

/* define if sys/types.h exists */
#define HAVE_SYS_TYPES_H 1

/* define if sys/utsname.h exists */
#define HAVE_SYS_UTSNAME_H 1

/* define if g2c.h exists */
/* #undef HAVE_G2C_H */

/* define if f2c.h exists */
/* #undef HAVE_F2C_H */

/* define if cblas.h exists */
#define HAVE_CBLAS_H 1

/* define if clapack.h exists */
/* #undef HAVE_CLAPACK_H */

/* define gid_t type */
/* #undef gid_t */

/* define off_t type */
/* #undef off_t */

/* define uid_t type */
/* #undef uid_t */

/* define if "long long int" is available */
#define HAVE_LONG_LONG_INT 1

/* define if "int64_t" is available */
#define HAVE_INT64_T 1

/* Define the return type of signal handlers */
#define RETSIGTYPE void

/* define if ftime() exists */
#define HAVE_FTIME 1

/* define if gethostname() exists */
#define HAVE_GETHOSTNAME 1

/* define if gettimeofday() exists */
#define HAVE_GETTIMEOFDAY 1

/* define if lseek() exists */
#define HAVE_LSEEK 1

/* define if time() exists */
#define HAVE_TIME 1

/* define if uname() exists */
#define HAVE_UNAME 1

/* define if seteuid() exists */
#define HAVE_SETEUID 1

/* define if setpriority() exists */
#define HAVE_SETPRIORITY 1

/* define if setreuid() exists */
#define HAVE_SETREUID 1

/* define if setruid() exists */
/* #undef HAVE_SETRUID */

/* define if setpgrp() takes no argument */
#define SETPGRP_VOID 1

/* define if drand48() exists */
#define HAVE_DRAND48 1

/* define if nanosleep() exists */
#define HAVE_NANOSLEEP 1

/* define if asprintf() exists */
#define HAVE_ASPRINTF 1

/* define if postgres is to be used */
#define HAVE_POSTGRES 1

/* define if SQLite is to be used */
#define HAVE_SQLITE 1

/* #undef USE_PROJ4API */
#ifndef USE_PROJ4API
/* define if proj.h exists and if the PROJ4 API should not be used */
#define HAVE_PROJ_H 1
#endif

/* define if GDAL is to be used */
#define HAVE_GDAL 1

/* define if OGR is to be used */
#define HAVE_OGR 1

/* define if GEOS is to be used */
#define HAVE_GEOS 1

/* define if postgres client header exists */
#define HAVE_LIBPQ_FE_H 1

/* define if PQcmdTuples in lpq */
#define HAVE_PQCMDTUPLES 1

/* define if ODBC exists */
/* #undef HAVE_SQL_H */

/* define if tiffio.h exists */
#define HAVE_TIFFIO_H 1

/* define if png.h exists */
#define HAVE_PNG_H 1

/* define if jpeglib.h exists */
/* #undef HAVE_JPEGLIB_H */

/* define if fftw3.h exists */
/* #undef HAVE_FFTW3_H */

/* define if fftw.h exists */
#define HAVE_FFTW_H 1

/* define if dfftw.h exists */
/* #undef HAVE_DFFTW_H */

/* define if BLAS exists */
#define HAVE_LIBBLAS 1

/* define if LAPACK exists */
#define HAVE_LIBLAPACK 1

/* define if ATLAS exists */
/* #undef HAVE_LIBATLAS */

/* define if dbm.h exists */
/* #undef HAVE_DBM_H */

/* define if readline exists */
/* #undef HAVE_READLINE_READLINE_H */

/* define if ft2build.h exists */
#define HAVE_FT2BUILD_H 1

/* Whether or not we are using G_socks for display communications */
/* #undef USE_G_SOCKS */

/* define if X is disabled or unavailable */
/* #undef X_DISPLAY_MISSING */

/* define if libintl.h exists */
#define HAVE_LIBINTL_H 1

/* define if iconv.h exists */
#define HAVE_ICONV_H 1

/* define if NLS requested */
#define USE_NLS 1

/* define if putenv() exists */
#define HAVE_PUTENV 1

/* define if setenv() exists */
#define HAVE_SETENV 1

/* define if socket() exists */
#define HAVE_SOCKET 1

/* define if glXCreatePbuffer exists */
#define HAVE_PBUFFERS 1

/* define if glXCreateGLXPixmap exists */
#define HAVE_PIXMAPS 1

/* define if OpenGL uses X11 */
#define OPENGL_X11 1

/* define if OpenGL uses Aqua (MacOS X) */
/* #undef OPENGL_AQUA */

/* define if OpenGL uses Windows */
/* #undef OPENGL_WINDOWS */

/* define if regex.h exists */
#define HAVE_REGEX_H 1

/* define if pthread.h exists */
#define HAVE_PTHREAD_H 1

/* define if fseeko() exists */
#define HAVE_FSEEKO 1

/*
 * configuration information solely dependent on the above
 * nothing below this point should need changing
 */

#if defined(HAVE_VALUES_H) && !defined(HAVE_LIMITS_H)
#define INT_MIN -MAXINT
#endif



/*
 * Defines needed to get large file support - from cdrtools-2.01
 */

/* MINGW32 LFS */

/* define if we have LFS */
#define	HAVE_LARGEFILES 1

#ifdef	HAVE_LARGEFILES		/* If we have working largefiles at all	   */
				/* This is not defined with glibc-2.1.3	   */

#if 0

/* what to do with these four? configure comments these out */

/* #undef _LARGEFILE_SOURCE */	/* To make ftello() visible (HP-UX 10.20). */
/* #undef _LARGE_FILES */		/* Large file defined on AIX-style hosts.  */
/* #undef _XOPEN_SOURCE */		/* To make ftello() visible (glibc 2.1.3). */
/* #undef _XOPEN_SOURCE_EXTENDED */
				/* XXX We don't use this because glibc2.1.3*/
				/* XXX is bad anyway. If we define	   */
				/* XXX _XOPEN_SOURCE we will loose caddr_t */

#endif

#if defined(__MINGW32__) && (!defined(_FILE_OFFSET_BITS) || (_FILE_OFFSET_BITS != 64))

/* add/remove as needed */
/* redefine off_t */
#include <sys/types.h>
#define off_t off64_t
/* fseeko and ftello are safe because not defined by MINGW */
#define HAVE_FSEEKO
#define fseeko fseeko64
#define ftello ftello64
/* redefine lseek */
#include <unistd.h>
#define lseek lseek64
/* redefine stat and fstat */
/* use _stati64 compatible with MSVCRT < 6.1 */
#include <sys/stat.h>
#define stat _stati64
#define fstat _fstati64

#endif /* MINGW32 LFS */

#endif	/* HAVE_LARGEFILES */


/* define if langinfo.h exists */
#define HAVE_LANGINFO_H 1

/* Use framebuffer objects for off-screen OpenGL rendering */
#define OPENGL_FBO 1

#endif /* _config_h */
