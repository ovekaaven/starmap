#include "maths.h"
#include <list>
#include <memory>
#include <vector>
#include <wx/app.h>
#include <wx/dcmemory.h>
#include <wx/frame.h>
#include <wx/list.h>

// some definitions

#define LIGHTYEAR_PER_PARSEC 3.26

// Sol is at least 14 light years above galactic plane,
// but I'm not sure which way. For now, pretend we're
// on the galactic plane.
#define SOL_Z_OFFSET 0

// other stuff we might need someday
#define ECLIPTIC 84381.412 // arcsec
#define LIGHTSEC 299792.458 // km/s
#define PRECESSION 5028.83  // arcsec/century

// the program's internal star representation

class starname {
 public:
  wxString name;
  int priority;
  starname(wxString nam, int pri) : name(nam), priority(pri) {}
  starname(const starname& other) : name(other.name), priority(other.priority) {}
  bool operator<(const starname& other) { return priority < other.priority; }
  bool operator>(const starname& other) { return priority > other.priority; }
};

class stardata {
 public:
  bool merged;
  std::list<starname> names;
  int comp;

  double x, y, z; // star coordinates (parsecs, heliocentric)
  wxPoint proj;   // current projection point
  bool show;      // current visibility
  wxCoord tw, th; // text extents
  bool te;        // text extents

  double vmag; // visual magnitude
  wxString type;  // spectral type
  wxColour color;

  double temp;

  wxString remarks; // remarks

  stardata(void)
    : merged(FALSE), te(FALSE) {}
  void sort_names(void);
  bool has_name(const wxString& name);

  void set_pos(const Vector&pos)
    { pos.get(x, y, z); }
  Vector get_pos(void)
    { return Vector(x, y, z); }

  // TODO: add other interesting stuff to keep track of


};

class starcomp {
 public:
  stardata *main;
  std::vector<stardata*> comp;

  starcomp(void)
    : main(NULL) {}
};

WX_DECLARE_STRING_HASH_MAP(starcomp*, starnamemap);

// the user interface

WX_DECLARE_LIST(stardata, starlist);

extern starlist stars;
extern starnamemap starnames;

class stardesc {
 public:
  const stardata *star;
  wxString desc;
  bool prepped;
  wxPoint pos;
  wxSize siz;
  stardesc(const stardata *st, const Vector& ref);
};

class StarApp : public wxApp
{
 public:
  StarApp(void);
  bool OnInit(void);
};

class StarCanvas;
class StarFrame : public wxFrame
{
 public:
  StarCanvas *canvas;

  StarFrame(wxFrame *parent, const char *title, int x, int y, int w, int h);

  void OnSize(wxSizeEvent& event);
  void OnCloseWindow(wxCloseEvent& event);

  void Quit(wxCommandEvent& event);
  void About(wxCommandEvent& event);
  void Option(wxCommandEvent& event);
  void Search(wxCommandEvent& event);

  DECLARE_EVENT_TABLE()
};

class StarCanvas : public wxWindow
{
 public:
  Vector pos, refpos;
  Angle pitch;
  double zoom;
  bool need_realloc, need_render, need_paint, ready;
  std::unique_ptr<wxBitmap> bmp;
  std::unique_ptr<wxMemoryDC> dc;

  std::list<const stardata*> select;
  std::list<stardesc> descs;
  wxPoint descpt;

  StarCanvas(wxFrame *parent);

  void OnSize(wxSizeEvent& event);
  void OnChar(wxKeyEvent& event);
  void OnKeyDown(wxKeyEvent& event);
  void OnMotion(wxMouseEvent& event);
  void OnLeaveWindow(wxMouseEvent& event);
  void OnLeftDown(wxMouseEvent& event);
  void OnIdle(wxIdleEvent& event);
  void OnPaint(wxPaintEvent& event);
  void RenderStars();
  void RenderView();
  void DoPaint(wxDC& pdc);
  void DoRepaint(void);
  void Redraw();
  void Repaint(bool clr_desc = TRUE);
  void CreateDescs(void);
  void ClearDescs(void);
  wxSize CalcBox(wxDC& pdc, wxString txt, int *tabpos = (int *)NULL);
  void ShowBox(wxDC& pdc, wxString txt, wxPoint pos);

  DECLARE_EVENT_TABLE()
};
