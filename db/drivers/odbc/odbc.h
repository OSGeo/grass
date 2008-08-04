#ifndef _ODBC_H_
#define	_ODBC_H_

#ifdef __MINGW32__
#include <windows.h>
#endif

/* configure checks for sql.h and stores in $(ODBCINC) */
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#endif

/* ********* unused:
 *#ifndef __FreeBSD__
 *#include <odbc/sql.h>
 *#include <odbc/sqlext.h>
 *#include <odbc/sqltypes.h>
 *
 *#else
 */
/* FreeBSD unixODBC port installs these header files in /usr/local/include */

/* ********* unused:
 *#include <sql.h>
 *#include <sqlext.h>
 *#include <sqltypes.h>
 *
 *#endif
 */
