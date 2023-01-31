#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "growing.h"

void P_Aux_to_Coor(struct Map_info *In, struct Map_info *Out, dbDriver *driver,
<<<<<<< HEAD
<<<<<<< HEAD
                   FILE *fsite UNUSED)
=======
                   FILE *fsite)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                   FILE *fsite)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    int more, ltype, line_num, ID_type, Interp_type;
    double quotaZ;

    struct line_pnts *point;
    struct line_cats *cat;
    dbTable *table;
    dbColumn *ID_column, *Interp_column;
    dbValue *ID_value, *Interp_value;
    dbCursor cursor;
    dbString sql;

    point = Vect_new_line_struct();
    cat = Vect_new_cats_struct();
    db_init_string(&sql);
    db_zero_string(&sql);

    db_append_string(&sql,
                     "select ID, sum(Interp) from Auxiliar_table group by ID");
    db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL);

    while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        table = db_get_cursor_table(&cursor);

        ID_column = db_get_table_column(table, 0);
        Interp_column = db_get_table_column(table, 1);

        ID_type = db_sqltype_to_Ctype(db_get_column_sqltype(ID_column));
        Interp_type = db_sqltype_to_Ctype(db_get_column_sqltype(Interp_column));

        if (ID_type == DB_C_TYPE_INT)
            ID_value = db_get_column_value(ID_column);
        else
            continue;

        if (Interp_type == DB_C_TYPE_DOUBLE)
            Interp_value = db_get_column_value(Interp_column);
        else
            continue;

        line_num = db_get_value_int(ID_value);
        quotaZ = db_get_value_double(Interp_value);

        ltype = Vect_read_line(In, point, cat, line_num);

        if (!(ltype & GV_POINT))
            continue;

=======
        cont++;

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
        table = db_get_cursor_table(&cursor);

        ID_column = db_get_table_column(table, 0);
        Interp_column = db_get_table_column(table, 1);

        ID_type = db_sqltype_to_Ctype(db_get_column_sqltype(ID_column));
        Interp_type = db_sqltype_to_Ctype(db_get_column_sqltype(Interp_column));

        if (ID_type == DB_C_TYPE_INT)
            ID_value = db_get_column_value(ID_column);
        else
            continue;

        if (Interp_type == DB_C_TYPE_DOUBLE)
            Interp_value = db_get_column_value(Interp_column);
        else
            continue;

        line_num = db_get_value_int(ID_value);
        quotaZ = db_get_value_double(Interp_value);

        ltype = Vect_read_line(In, point, cat, line_num);

        if (!(ltype & GV_POINT))
            continue;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        cont++;

        table = db_get_cursor_table(&cursor);

        ID_column = db_get_table_column(table, 0);
        Interp_column = db_get_table_column(table, 1);

        ID_type = db_sqltype_to_Ctype(db_get_column_sqltype(ID_column));
        Interp_type = db_sqltype_to_Ctype(db_get_column_sqltype(Interp_column));

        if (ID_type == DB_C_TYPE_INT)
            ID_value = db_get_column_value(ID_column);
        else
            continue;

        if (Interp_type == DB_C_TYPE_DOUBLE)
            Interp_value = db_get_column_value(Interp_column);
        else
            continue;

        line_num = db_get_value_int(ID_value);
        quotaZ = db_get_value_double(Interp_value);

        ltype = Vect_read_line(In, point, cat, line_num);

        if (!(ltype & GV_POINT))
            continue;

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
        table = db_get_cursor_table(&cursor);

        ID_column = db_get_table_column(table, 0);
        Interp_column = db_get_table_column(table, 1);

        ID_type = db_sqltype_to_Ctype(db_get_column_sqltype(ID_column));
        Interp_type = db_sqltype_to_Ctype(db_get_column_sqltype(Interp_column));

        if (ID_type == DB_C_TYPE_INT)
            ID_value = db_get_column_value(ID_column);
        else
            continue;

        if (Interp_type == DB_C_TYPE_DOUBLE)
            Interp_value = db_get_column_value(Interp_column);
        else
            continue;

        line_num = db_get_value_int(ID_value);
        quotaZ = db_get_value_double(Interp_value);

        ltype = Vect_read_line(In, point, cat, line_num);

        if (!(ltype & GV_POINT))
            continue;

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        point->z[0] = quotaZ;
        Vect_write_line(Out, ltype, point, cat);
    }
    return;
}
