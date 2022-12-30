extern int db_d_add_column(void);
extern int db_d_bind_update(void);
extern int db_d_close_cursor(void);
extern int db_d_close_database(void);
extern int db_d_create_database(void);
extern int db_d_create_index(void);
extern int db_d_create_table(void);
extern int db_d_delete_database(void);
extern int db_d_describe_table(void);
extern int db_d_drop_column(void);
extern int db_d_drop_index(void);
extern int db_d_drop_table(void);
extern int db_d_execute_immediate(void);
extern int db_d_begin_transaction(void);
extern int db_d_commit_transaction(void);
extern int db_d_fetch(void);
extern int db_d_get_num_rows(void);
extern int db_d_find_database(void);
extern int db_d_grant_on_table(void);
extern int db_d_insert(void);
extern int db_d_delete(void);
extern int db_d_list_databases(void);
extern int db_d_list_indexes(void);
extern int db_d_list_tables(void);
extern int db_d_open_database(void);
extern int db_d_open_insert_cursor(void);
extern int db_d_open_select_cursor(void);
extern int db_d_open_update_cursor(void);
extern int db_d_update(void);
extern int db_d_version(void);

static struct {
    int procnum;
<<<<<<< HEAD
    int (*routine)(void);
=======
    int (*routine)();
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
} procedure[] = {{DB_PROC_FETCH, db_d_fetch},
                 {DB_PROC_ROWS, db_d_get_num_rows},
                 {DB_PROC_UPDATE, db_d_update},
                 {DB_PROC_INSERT, db_d_insert},
                 {DB_PROC_DELETE, db_d_delete},
                 {DB_PROC_EXECUTE_IMMEDIATE, db_d_execute_immediate},
                 {DB_PROC_BEGIN_TRANSACTION, db_d_begin_transaction},
                 {DB_PROC_COMMIT_TRANSACTION, db_d_commit_transaction},
                 {DB_PROC_OPEN_SELECT_CURSOR, db_d_open_select_cursor},
                 {DB_PROC_OPEN_UPDATE_CURSOR, db_d_open_update_cursor},
                 {DB_PROC_BIND_UPDATE, db_d_bind_update},
                 {DB_PROC_OPEN_INSERT_CURSOR, db_d_open_insert_cursor},
                 {DB_PROC_CLOSE_CURSOR, db_d_close_cursor},
                 {DB_PROC_LIST_TABLES, db_d_list_tables},
                 {DB_PROC_DESCRIBE_TABLE, db_d_describe_table},
                 {DB_PROC_CREATE_TABLE, db_d_create_table},
                 {DB_PROC_DROP_TABLE, db_d_drop_table},
                 {DB_PROC_GRANT_ON_TABLE, db_d_grant_on_table},
                 {DB_PROC_OPEN_DATABASE, db_d_open_database},
                 {DB_PROC_CLOSE_DATABASE, db_d_close_database},
                 {DB_PROC_LIST_DATABASES, db_d_list_databases},
                 {DB_PROC_CREATE_DATABASE, db_d_create_database},
                 {DB_PROC_DELETE_DATABASE, db_d_delete_database},
                 {DB_PROC_FIND_DATABASE, db_d_find_database},
                 {DB_PROC_CREATE_INDEX, db_d_create_index},
                 {DB_PROC_DROP_INDEX, db_d_drop_index},
                 {DB_PROC_LIST_INDEXES, db_d_list_indexes},
                 {DB_PROC_ADD_COLUMN, db_d_add_column},
                 {DB_PROC_DROP_COLUMN, db_d_drop_column},
                 {DB_PROC_VERSION, db_d_version},
                 {-1, NULL}};
