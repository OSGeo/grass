/*!
  \file lib/db/dbmi_base/error.c
  
  \brief DBMI Library (base) - error management
  
  (C) 1999-2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC)
  \author Upgraded to GRASS 5.7 by Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

static int err_flag = 0;
static int err_code = DB_OK;
static char *err_msg = 0;
static int auto_print_errors = 1;
static int auto_print_protocol_errors = 1;
static void (*user_print_function) (const char *);

static char *who = NULL;

/*!
  \brief User defined error procedure

  \param f pointer to user-defined function
*/
void db_on_error(void (*f) (const char *))
{
    user_print_function = f;
}

/*!
  \brief Set 'who' for error messages

  \param me my name
*/
void db_set_error_who(const char *me)
{
    if (who)
	db_free(who);
    who = db_store(me);
}

/*!
  brief Get 'who' string

  \return pointer to string buffer
  \return empty buffer if 'who' is not defined
*/
const char *db_get_error_who(void)
{
    return who ? who : "";
}

/*!
  \brief Report error message
  
  \param s error message (can be NULL)
*/
void db_error(const char *s)
{
    if (s == NULL)
	s = _("<NULL error message>");
    if (err_msg)
	db_free(err_msg);
    err_msg = db_store(s);
    err_flag = 1;
    if (auto_print_errors)
	db_print_error();
    err_code = DB_FAILED;
}

/*!
  \brief Report protocol error
*/
void db_protocol_error(void)
{
    int flag;

    flag = auto_print_errors;
    auto_print_errors = auto_print_protocol_errors;
    db_error(_("dbmi: Protocol error"));
    auto_print_errors = flag;
    err_code = DB_PROTOCOL_ERR;
}

/*!
  \brief Report system error
  
  \param s error message
*/
void db_syserror(const char *s)
{
    char lead[1024];
    char msg[1024];


    err_flag = 0;
    if (errno <= 0)
	return;

    *lead = 0;
    if (who)
	sprintf(lead, "%s: ", who);

    if (errno > 0)
	sprintf(msg, "%s%s: %s", lead, strerror(errno), s);

    db_error(msg);
}

/*!
  \brief Get error code
  
  \return DB_OK if not defined
 */
int db_get_error_code(void)
{
    return err_flag ? err_code : DB_OK;
}

/*!
  \brief  Report memory error
*/
void db_memory_error(void)
{
    db_error(_("dbmi: Out of Memory"));
    err_code = DB_MEMORY_ERR;
}

/*!
  \brief Report 'not implemented' error

  \param name name of functionality
*/
void db_procedure_not_implemented(const char *name)
{
    char msg[128];

    sprintf(msg, _("dbmi: %s() not implemented"), name);
    db_error(msg);
    err_code = DB_NOPROC;
}

/*!
  \brief Report no procedure error

  \param procnum procedure number
*/
void db_noproc_error(int procnum)
{
    char msg[128];

    sprintf(msg, _("dbmi: Invalid procedure %d"), procnum);
    db_error(msg);
    err_code = DB_NOPROC;
}

/*!
  \brief Clear error status
*/
void db_clear_error(void)
{
    err_flag = 0;
    err_code = DB_OK;
    errno = 0;			/* clearn system errno as well */
}

/*!
  \brief Print error

  If not defined, the error message is printed to stderr.
*/
void db_print_error(void)
{
    char lead[1024];

    if (!err_flag)
	return;

    *lead = 0;
    if (who)
	sprintf(lead, "%s: ", who);

    if (user_print_function) {
	char buf[1024];

	sprintf(buf, "%s%s\n", lead, err_msg);
	user_print_function(buf);
    }
    else
	fprintf(stderr, "%s%s\n", lead, err_msg);
}


static int debug_on = 0;

/*!
  \brief Turn on debugging
*/
void db_debug_on(void)
{
    debug_on = 1;
}

/*!
  \brief Turn off debugging
*/
void db_debug_off(void)
{
    debug_on = 0;
}

/*!
  \brief Print debug message

  \param s debug message
*/
void db_debug(const char *s)
{
    if (debug_on)
	fprintf(stderr, "debug(%s): %s\n", who ? who : "", s ? s : "<NULL>");
}

/*!
  \brief Get error message

  \return pointer to error message string
*/
const char *db_get_error_msg(void)
{
    return err_flag ? err_msg : (const char *)NULL;
}

/*!
  \brief Toggles printing of DBMI error messages

  \param flag ?
*/
void db_auto_print_errors(int flag)
{
    auto_print_errors = flag;
    auto_print_protocol_errors = flag;
}

/*!
  \brief Set auto print protocol error

  \param flag ?
 */
void db_auto_print_protocol_errors(int flag)
{
    auto_print_protocol_errors = flag;
}
