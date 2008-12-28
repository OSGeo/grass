#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
   \brief returns column structure for given table and column number
   \return 
   \param 
 */
dbColumn *db_get_table_column(dbTable * table, int n)
{
    if (n < 0 || n >= table->numColumns)
	return ((dbColumn *) NULL);
    return &table->columns[n];
}

/*!
   \brief returns column value for given column structure
   \return 
   \param 
 */
dbValue *db_get_column_value(dbColumn * column)
{
    return &column->value;
}

/*!
   \brief returns column default value for given column structure
   \return 
   \param 
 */
dbValue *db_get_column_default_value(dbColumn * column)
{
    return &column->defaultValue;
}

/*!
   \brief define column sqltype for column (the function db_sqltype_name() 
   returns sqltype description)
   \return 
   \param 
 */
void db_set_column_sqltype(dbColumn * column, int sqltype)
{
    column->sqlDataType = sqltype;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_host_type(dbColumn * column, int type)
{
    column->hostDataType = type;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_get_column_scale(dbColumn * column)
{
    return column->scale;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_scale(dbColumn * column, int scale)
{
    column->scale = scale;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_get_column_precision(dbColumn * column)
{
    return column->precision;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_precision(dbColumn * column, int precision)
{
    column->precision = precision;
}

/*!
   \brief returns column sqltype for column (the function db_sqltype_name() 
   returns sqltype description)
   \return 
   \param 
 */
int db_get_column_sqltype(dbColumn * column)
{
    return column->sqlDataType;
}

int db_get_column_host_type(dbColumn * column)
{
    return column->hostDataType;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_has_defined_default_value(dbColumn * column)
{
    column->hasDefaultValue = 1;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_has_undefined_default_value(dbColumn * column)
{
    column->hasDefaultValue = 0;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_unset_column_has_default_value(dbColumn * column)
{
    column->hasDefaultValue = 0;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_test_column_has_default_value(dbColumn * column)
{
    return (column->hasDefaultValue != 0);
}

/*!
   \brief 
   \return 
   \param 
 */
int db_test_column_has_defined_default_value(dbColumn * column)
{
    return (column->hasDefaultValue);
}

/*!
   \brief 
   \return 
   \param 
 */
int db_test_column_has_undefined_default_value(dbColumn * column)
{
    return (!column->hasDefaultValue);
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_use_default_value(dbColumn * column)
{
    column->useDefaultValue = 1;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_unset_column_use_default_value(dbColumn * column)
{
    column->useDefaultValue = 0;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_test_column_use_default_value(dbColumn * column)
{
    return (column->useDefaultValue != 0);
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_null_allowed(dbColumn * column)
{
    column->nullAllowed = 1;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_unset_column_null_allowed(dbColumn * column)
{
    column->nullAllowed = 0;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_test_column_null_allowed(dbColumn * column)
{
    return (column->nullAllowed != 0);
}

/*!
   \brief 
   \return 
   \param 
 */
int db_get_column_length(dbColumn * column)
{
    return column->dataLen;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_length(dbColumn * column, int length)
{
    column->dataLen = length;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_select_priv_granted(dbColumn * column)
{
    column->select = DB_GRANTED;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_select_priv_not_granted(dbColumn * column)
{
    column->select = DB_NOT_GRANTED;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_get_column_select_priv(dbColumn * column)
{
    return column->select;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_update_priv_granted(dbColumn * column)
{
    column->update = DB_GRANTED;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_set_column_update_priv_not_granted(dbColumn * column)
{
    column->update = DB_NOT_GRANTED;
}

/*!
   \brief 
   \return 
   \param 
 */
int db_get_column_update_priv(dbColumn * column)
{
    return column->update;
}

/*!
   \brief 
   \return 
   \param 
 */
void db_init_column(dbColumn * column)
{
    db_zero((void *)column, sizeof(dbColumn));
    db_init_string(&column->columnName);
    db_init_string(&column->description);
    db_init_string(&column->value.s);
    db_init_string(&column->defaultValue.s);
}

/*!
   \brief 
   \return 
   \param 
 */
int db_set_column_name(dbColumn * column, const char *name)
{
    return db_set_string(&column->columnName, name);
}

/*!
   \brief returns column name for given column
   \return 
   \param 
 */
const char *db_get_column_name(dbColumn * column)
{
    return db_get_string(&column->columnName);
}

/*!
   \brief 
   \return 
   \param 
 */
int db_set_column_description(dbColumn * column, const char *description)
{
    return db_set_string(&column->description, description);
}

/*!
   \brief returns column description for given column
   \return 
   \param 
 */
const char *db_get_column_description(dbColumn * column)
{
    return db_get_string(&column->description);
}

/*!
   \brief frees column structure
   \return 
   \param 
 */
void db_free_column(dbColumn * column)
{
    db_free_string(&column->columnName);
    db_free_string(&column->value.s);
    /* match init? */
    db_free_string(&column->description);
    db_free_string(&column->defaultValue.s);
}
