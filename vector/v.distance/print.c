#include "local_proto.h"

/*
   print out upload values
 */
int print_upload(NEAR *Near, UPLOAD *Upload, int i, dbCatValArray *cvarr,
                 dbCatVal *catval, char *sep, enum OutputFormat format,
                 JSON_Object *object)
{
    JSON_Value *relations_value, *relation_value;
    JSON_Array *relations;
    JSON_Object *relation_object;
    char *name = NULL;

    if (format == JSON) {
        relations_value = json_value_init_array();
        relations = json_array(relations_value);
    }

    int j;

    j = 0;
    while (Upload[j].upload != END) {
        if (Near[i].count == 0) { /* no nearest found */
            fprintf(stdout, "%snull", sep);
        }
        else {
            switch (format) {
            case JSON:
                relation_value = json_value_init_object();
                relation_object = json_object(relation_value);
                switch (Upload[j].upload) {
                case CAT:
                    name = "to_cat";
                    if (Near[i].to_cat >= 0)
                        json_object_set_number(relation_object, "value",
                                               Near[i].to_cat);
                    else {
                        json_object_set_null(relation_object, "value");
                    }
                    break;
                case DIST:
                    name = "dist";
                    json_object_set_number(relation_object, "value",
                                           Near[i].dist);
                    break;
                case FROM_X:
                    name = "from_x";
                    json_object_set_number(relation_object, "value",
                                           Near[i].from_x);
                    break;
                case FROM_Y:
                    name = "from_y";
                    json_object_set_number(relation_object, "value",
                                           Near[i].from_y);
                    break;
                case TO_X:
                    name = "to_x";
                    json_object_set_number(relation_object, "value",
                                           Near[i].to_x);
                    break;
                case TO_Y:
                    name = "to_y";
                    json_object_set_number(relation_object, "value",
                                           Near[i].to_y);
                    break;
                case FROM_ALONG:
                    name = "from_along";
                    json_object_set_number(relation_object, "value",
                                           Near[i].from_along);
                    break;
                case TO_ALONG:
                    name = "to_along";
                    json_object_set_number(relation_object, "value",
                                           Near[i].to_along);
                    break;
                case TO_ANGLE:
                    name = "to_angle";
                    json_object_set_number(relation_object, "value",
                                           Near[i].to_angle);
                    break;
                case TO_ATTR:
                    name = "to_attr";
                    if (catval) {
                        switch (cvarr->ctype) {
                        case DB_C_TYPE_INT:
                            json_object_set_number(relation_object, "value",
                                                   catval->val.i);
                            break;
                        case DB_C_TYPE_DOUBLE:
                            json_object_set_number(relation_object, "value",
                                                   catval->val.d);
                            break;
                        case DB_C_TYPE_STRING:
                            json_object_set_string(
                                relation_object, "value",
                                db_get_string(catval->val.s));
                            break;
                        case DB_C_TYPE_DATETIME:
                            /* TODO: formatting datetime */
                            break;
                        }
                    }
                    else {
                        json_object_set_null(relation_object, "value");
                    }
                    break;
                default:
                    break;
                }
                json_object_set_string(relation_object, "name", name);
                json_array_append_value(relations, relation_value);
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

    if (format == JSON) {
        json_object_set_value(object, "distances", relations_value);
    }
    return 0;
}
