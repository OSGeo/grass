#include "local_proto.h"

/*
   print out upload values
 */
int print_upload(NEAR *Near, UPLOAD *Upload, int i, dbCatValArray *cvarr,
                 dbCatVal *catval, char *sep, enum OutputFormat format,
                 G_JSON_Object *object)
{
    int j;

    j = 0;
    while (Upload[j].upload != END) {
        if (Near[i].count == 0) { /* no nearest found */
            if (format == PLAIN)
                fprintf(stdout, "%snull", sep);
        }
        else {
            switch (format) {
            case JSON:
                switch (Upload[j].upload) {
                case CAT:
                    if (Near[i].to_cat >= 0)
                        G_json_object_set_number(object, "to_cat",
                                                 Near[i].to_cat);
                    else {
                        G_json_object_set_null(object, "to_cat");
                    }
                    break;
                case DIST:
                    G_json_object_set_number(object, "dist", Near[i].dist);
                    break;
                case FROM_X:
                    G_json_object_set_number(object, "from_x", Near[i].from_x);
                    break;
                case FROM_Y:
                    G_json_object_set_number(object, "from_y", Near[i].from_y);
                    break;
                case TO_X:
                    G_json_object_set_number(object, "to_x", Near[i].to_x);
                    break;
                case TO_Y:
                    G_json_object_set_number(object, "to_y", Near[i].to_y);
                    break;
                case FROM_ALONG:
                    G_json_object_set_number(object, "from_along",
                                             Near[i].from_along);
                    break;
                case TO_ALONG:
                    G_json_object_set_number(object, "to_along",
                                             Near[i].to_along);
                    break;
                case TO_ANGLE:
                    G_json_object_set_number(object, "to_angle",
                                             Near[i].to_angle);
                    break;
                case TO_ATTR:
                    if (catval) {
                        switch (cvarr->ctype) {
                        case DB_C_TYPE_INT:
                            G_json_object_set_number(object, "to_attr",
                                                     catval->val.i);
                            break;
                        case DB_C_TYPE_DOUBLE:
                            G_json_object_set_number(object, "to_attr",
                                                     catval->val.d);
                            break;
                        case DB_C_TYPE_STRING:
                            G_json_object_set_string(
                                object, "to_attr",
                                db_get_string(catval->val.s));
                            break;
                        case DB_C_TYPE_DATETIME:
                            /* TODO: formatting datetime */
                            break;
                        }
                    }
                    else {
                        G_json_object_set_null(object, "to_attr");
                    }
                    break;
                default:
                    break;
                }
                break;
            case PLAIN:
                switch (Upload[j].upload) {
                case CAT:
                    if (Near[i].to_cat >= 0)
                        fprintf(stdout, "%s%d", sep, Near[i].to_cat);
                    else
                        fprintf(stdout, "%snull", sep);
                    break;
                case DIST:
                    fprintf(stdout, "%s%.17g", sep, Near[i].dist);
                    break;
                case FROM_X:
                    fprintf(stdout, "%s%.17g", sep, Near[i].from_x);
                    break;
                case FROM_Y:
                    fprintf(stdout, "%s%.17g", sep, Near[i].from_y);
                    break;
                case TO_X:
                    fprintf(stdout, "%s%.17g", sep, Near[i].to_x);
                    break;
                case TO_Y:
                    fprintf(stdout, "%s%.17g", sep, Near[i].to_y);
                    break;
                case FROM_ALONG:
                    fprintf(stdout, "%s%.17g", sep, Near[i].from_along);
                    break;
                case TO_ALONG:
                    fprintf(stdout, "%s%.17g", sep, Near[i].to_along);
                    break;
                case TO_ANGLE:
                    fprintf(stdout, "%s%f", sep, Near[i].to_angle);
                    break;
                case TO_ATTR:
                    if (catval) {
                        switch (cvarr->ctype) {
                        case DB_C_TYPE_INT:
                            fprintf(stdout, "%s%d", sep, catval->val.i);
                            break;

                        case DB_C_TYPE_DOUBLE:
                            fprintf(stdout, "%s%.17g", sep, catval->val.d);
                            break;

                        case DB_C_TYPE_STRING:
                            fprintf(stdout, "%s%s", sep,
                                    db_get_string(catval->val.s));
                            break;

                        case DB_C_TYPE_DATETIME:
                            /* TODO: formatting datetime */
                            fprintf(stdout, "%s", sep);
                            break;
                        }
                    }
                    else {
                        fprintf(stdout, "%snull", sep);
                    }
                    break;
                default:
                    break;
                }
                break;
            }
        }
        j++;
    }

    return 0;
}
