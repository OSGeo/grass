#include "interface.h"
#include "string.h"
#include "coldefs.h"

vect_translate(id, type, data, interp)
     int id, type;
     Nv_data *data;
     Tcl_Interp *interp;
{
    /* Currently I can find no routine which uses this so I'll take it out
       vect_data *vect;

       vect = &(data->vect[data->CurVect]);

       GV_set_trans(vect->id, vect->xtrans, vect->ytrans, vect->ztrans);
     */ ;
}
