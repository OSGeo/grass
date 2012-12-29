#include "local_proto.h"

/*
   print out upload values 
 */
int print_upload(NEAR * Near, UPLOAD * Upload, int i,
		 dbCatValArray * cvarr, dbCatVal * catval)
{
    int j;

    j = 0;
    while (Upload[j].upload != END) {
	if (Near[i].count == 0) {	/* no nearest found */
	    fprintf(stdout, "|null");
	}
	else {
	    switch (Upload[j].upload) {
	    case CAT:
		if (Near[i].to_cat >= 0)
		    fprintf(stdout, "|%d", Near[i].to_cat);
		else
		    fprintf(stdout, "|null");
		break;
	    case DIST:
		fprintf(stdout, "|%.17g", Near[i].dist);
		break;
	    case FROM_X:
		fprintf(stdout, "|%.17g", Near[i].from_x);
		break;
	    case FROM_Y:
		fprintf(stdout, "|%.17g", Near[i].from_y);
		break;
	    case TO_X:
		fprintf(stdout, "|%.17g", Near[i].to_x);
		break;
	    case TO_Y:
		fprintf(stdout, "|%.17g", Near[i].to_y);
		break;
	    case FROM_ALONG:
		fprintf(stdout, "|%.17g", Near[i].from_along);
		break;
	    case TO_ALONG:
		fprintf(stdout, "|%.17g", Near[i].to_along);
		break;
	    case TO_ANGLE:
		fprintf(stdout, "|%f", Near[i].to_angle);
		break;
	    case TO_ATTR:
		if (catval) {
		    switch (cvarr->ctype) {
		    case DB_C_TYPE_INT:
			fprintf(stdout, "|%d", catval->val.i);
			break;

		    case DB_C_TYPE_DOUBLE:
			fprintf(stdout, "|%.17g", catval->val.d);
			break;

		    case DB_C_TYPE_STRING:
			fprintf(stdout, "|%s", db_get_string(catval->val.s));
			break;

		    case DB_C_TYPE_DATETIME:
			/* TODO: formating datetime */
			fprintf(stdout, "|");
			break;
		    }
		}
		else {
		    fprintf(stdout, "|null");
		}
		break;
	    }
	}
	j++;
    }

    return 0;
}
