
/**********************************************************************
   std_incs.h       - header file for standard includes
 *********************************************************************/
#ifndef STD_INCS_H
#define STD_INCS_H

#include <X11/Xos.h>
extern char *calloc();		/* removed <malloc.h> GRASS930313 parghi 1993-03-30 */
extern char *malloc();
extern char *realloc();

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#if defined(sparc) || defined (uts) || defined(SVR4) || defined(IGRAPH)
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#ifndef SVR4
#include <sys/wait.h>
#endif
#ifndef mips
#include <unistd.h>
#include <stdlib.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/BulletinB.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/CutPaste.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawnB.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>

#define XG_FAIL    0
#define XG_SUCCESS 1

#endif /* STD_INCS_H */
