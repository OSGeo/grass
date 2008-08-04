extern int db_d_add_column();
extern int db_d_bind_update();
extern int db_d_close_cursor();
extern int db_d_close_database();
extern int db_d_create_database();
extern int db_d_create_index();
extern int db_d_create_table();
extern int db_d_delete_database();
extern int db_d_describe_table();
extern int db_d_drop_column();
extern int db_d_drop_index();
extern int db_d_drop_table();
extern int db_d_execute_immediate();
extern int db_d_begin_transaction();
extern int db_d_commit_transaction();
extern int db_d_fetch();
extern int db_d_get_num_rows();
extern int db_d_find_database();
extern int db_d_grant_on_table();
extern int db_d_insert();
extern int db_d_delete();
extern int db_d_list_databases();
extern int db_d_list_indexes();
extern int db_d_list_tables();
extern int db_d_open_database();
extern int db_d_open_insert_cursor();
extern int db_d_open_select_cursor();
extern int db_d_open_update_cursor();
extern int db_d_update();
extern int db_d_version();

static struct
{
    int procnum;
    int (*routine) ();
} procedure[] = {
    {
    DB_PROC_FETCH, db_d_fetch}, {
    DB_PROC_ROWS, db_d_get_num_rows}, {
    DB_PROC_UPDATE, db_d_update}, {
    DB_PROC_INSERT, db_d_insert}, {
    DB_PROC_DELETE, db_d_delete}, {
    DB_PROC_EXECUTE_IMMEDIATE, db_d_execute_immediate}, {
    DB_PROC_BEGIN_TRANSACTION, db_d_begin_transaction}, {
    DB_PROC_COMMIT_TRANSACTION, db_d_commit_transaction}, {
    DB_PROC_OPEN_SELECT_CURSOR, db_d_open_select_cursor}, {
    DB_PROC_OPEN_UPDATE_CURSOR, db_d_open_update_cursor}, {
    DB_PROC_BIND_UPDATE, db_d_bind_update}, {
    DB_PROC_OPEN_INSERT_CURSOR, db_d_open_insert_cursor}, {
    DB_PROC_CLOSE_CURSOR, db_d_close_cursor}, {
    DB_PROC_LIST_TABLES, db_d_list_tables}, {
    DB_PROC_DESCRIBE_TABLE, db_d_describe_table}, {
    DB_PROC_CREATE_TABLE, db_d_create_table}, {
    DB_PROC_DROP_TABLE, db_d_drop_table}, {
    DB_PROC_GRANT_ON_TABLE, db_d_grant_on_table}, {
    DB_PROC_OPEN_DATABASE, db_d_open_database}, {
    DB_PROC_CLOSE_DATABASE, db_d_close_database}, {
    DB_PROC_LIST_DATABASES, db_d_list_databases}, {
    DB_PROC_CREATE_DATABASE, db_d_create_database}, {
    DB_PROC_DELETE_DATABASE, db_d_delete_database}, {
    DB_PROC_FIND_DATABASE, db_d_find_database}, {
    DB_PROC_CREATE_INDEX, db_d_create_index}, {
    DB_PROC_DROP_INDEX, db_d_drop_index}, {
    DB_PROC_LIST_INDEXES, db_d_list_indexes}, {
    DB_PROC_ADD_COLUMN, db_d_add_column}, {
    DB_PROC_DROP_COLUMN, db_d_drop_column}, {
    DB_PROC_VERSION, db_d_version}, {
    -1, NULL}
};
