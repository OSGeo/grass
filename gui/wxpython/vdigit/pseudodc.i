
%{
#include <wx/wxPython/wxPython.h>
#include <wx/wxPython/pyclasses.h>
%}

%{
#include "pseudodc.h"
%}

%rename(PseudoDC) gwxPseudoDC;

%typemap(out) wxRect {
	$result = Py_BuildValue("iiii", $1.x, $1.y, $1.width, $1.height);
}

class gwxPseudoDC
{
public:
	gwxPseudoDC();
	~gwxPseudoDC();
	void Clear();
	void RemoveAll();
	void RemoveId(int id);
	%extend {
		void DrawToDC(void *dc) {
			self->DrawToDC((wxDC *) dc);
		}
		void DrawToDCClipped(void *dc, const wxRect& rect) {
			self->DrawToDCClipped((wxDC *) dc, rect);
		}
		wxRect GetIdBounds(int id) {
			wxRect rect;
			self->GetIdBounds(id, rect);
			return rect;
		}
	}
};

