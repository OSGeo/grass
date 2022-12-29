#include "local_proto.h"

/*
   print out upload values
 */
int print_upload(NEAR * Near, UPLOAD * Upload, int i,
		 dbCatValArray * cvarr, dbCatVal * catval, char *sep)
{
    int j;

    j = 0;
    while (Upload[j].upload != END) {
	if (Near[i].count == 0) {	/* no nearest found */
	    fprintf(stdout, "%snull", sep);
	}
	else {
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
			fprintf(stdout, "%s%s", sep, db_get_string(catval->val.s));
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
	    }
	}
	j++;
    }

    return 0;
}
