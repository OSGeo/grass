#ifndef __NVIZ_H__
#define __NVIZ_H__

#include <vector>

extern "C" {
#include <grass/gis.h>
#include <grass/gsurf.h>
#include <grass/gstypes.h>
#include <grass/nviz.h>
}

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include <wx/glcanvas.h>

#define VIEW_DEFAULT_POS_X 0.85
#define VIEW_DEFAULT_POS_Y 0.85
#define VIEW_DEFAULT_PERSP 40.0
#define VIEW_DEFAULT_TWIST 0.0

class Nviz
{
private:
    nv_data *data;
    wxGLCanvas *glCanvas;

    /* surface.cpp */
    int SetSurfaceAttr(int, int, bool, const char *);
    int UnsetSurfaceAttr(int, int);

public:
    /* constructor */
    Nviz();

    /* destructor */
    ~Nviz();

    /* change_view.cpp */
    int ResizeWindow(int, int);
    std::vector<double> SetViewDefault();
    int SetView(float, float,
		float, float, float);
    int SetZExag(float);

    /* init.cpp */
    int SetDisplay(void *);
    void InitView();
    void Reset();

    /* lights.cpp */
    void SetLightsDefault();

    /* load.cpp */
    int LoadSurface(const char*, const char *, const char *);
    int UnloadSurface(int);
    int LoadVector(const char *);
    int UnloadVector(int);

    /* draw.cpp */
    void Draw(bool);
    void EraseMap();

    /* surface.cpp */
    int SetSurfaceTopo(int, bool, const char *);
    int SetSurfaceColor(int, bool, const char *);
    int SetSurfaceMask(int, bool, const char *);
    int SetSurfaceTransp(int, bool, const char *);
    int SetSurfaceShine(int, bool, const char *);
    int SetSurfaceEmit(int, bool, const char *);
    int UnsetSurfaceMask(int);
    int UnsetSurfaceTransp(int);
    int UnsetSurfaceEmit(int);
    int SetSurfaceRes(int, int, int);
    int SetSurfaceStyle(int, int);
    int SetWireColor(int, const char *);
};

#endif /* __NVIZ_H__ */
