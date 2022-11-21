#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/list.h>
#endif

#include <list>

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

// some inline classes for doing astro-maths

#include <math.h>

class angle
{
 protected:
  double dat;
 public:
  angle(double r) { dat = r; } // angle (radians)
  operator double() const { return dat; }
  double operator+=(double d) { return dat += d; }
  double operator-=(double d) { return dat -= d; }
  double operator=(double d) { return dat = d; }
};

class angle_ra : public angle // right ascension (hours)
{
 public:
  angle_ra(unsigned h, double m)
    : angle( ((h*60) + m) * M_PI / (12*60) ) {}
  angle_ra(unsigned h, unsigned m, double s)
    : angle( ((h*3600) + (m*60) + s) * M_PI / (12*3600) ) {}
};

class angle_de : public angle // declination (degrees)
{
 public:
  angle_de(unsigned d, double m, char sg)
    : angle( ((d*60) + m) * M_PI / (180*60) )
    {if (sg == '-') { dat = -dat; }}
  angle_de(unsigned d, unsigned m, double s, char sg)
    : angle( ((d*3600) + (m*60) + s) * M_PI / (180*3600) )
    {if (sg == '-') { dat = -dat; }}
};

class coords;
class distance {
 protected:
  double dat;
 public:
  distance(double d) { dat = d; } // plain distance
  distance(coords&d); // vector
  operator double() const { return dat; }
  double operator=(double d) { return dat = d; }
};

class dist_ps : public distance // parsecs
{
 public:
  dist_ps(double d)
    : distance(d) {}
  dist_ps(unsigned pb, double plx) // parallax
    : distance(pb/plx) {}
  dist_ps(coords&d)
    : distance(d) {}
};

class dist_ly : public distance // lightyears
{
 public:
  dist_ly(double d)
    : distance(d) {}
  dist_ly(dist_ps& d)
    : distance(d*LIGHTYEAR_PER_PARSEC) {}
  dist_ly(coords&d)
    : distance(d) {}
};

class tmatrix;
class coords {
 protected:
  double cx, cy, cz;
 public:
  coords(void)
    { cx = 0; cy = 0; cz = 0; }
  coords(double x, double y, double z)
    { cx = x; cy = y; cz = z; }
  coords(angle phi, angle theta, distance rho)
  {
    double rvect = rho * cos(theta);
    cx = rvect * cos(phi);
    cy = rvect * sin(phi);
    cz = rho * sin(theta);
  }
  void flatten(void) { cz = 0; }
  void get(double&x,double&y,double&z) const
    { x = cx; y = cy; z = cz; }
  bool behind(void) const
    { return cz < 0.1; }
  double depth(void) const { return cz; }
  double sqr(void) const
    { return cx*cx + cy*cy + cz*cz; }
  double sqrt(void) const { return ::sqrt(sqr()); }
  void normalize(void) { *this /= sqrt(); }
  wxPoint project(double s, int xc, int yc) const // orthogonal projection
    { return wxPoint(s*cx + xc, s*cy + yc); }
  wxPoint pproject(double s, int xc, int yc) const // perspective projection
    { return wxPoint(s*cx/cz + xc, s*cy/cz + yc); }
  coords operator-(const coords& c) const
    { return coords(cx - c.cx, cy - c.cy, cz - c.cz); }
  coords operator+(const coords& c) const
    { return coords(cx + c.cx, cy + c.cy, cz + c.cz); }
  coords& operator+=(const coords& c)
    { cx += c.cx; cy += c.cy; cz += c.cz; return *this; }
  coords& operator-=(const coords& c)
    { cx -= c.cx; cy -= c.cy; cz -= c.cz; return *this; }
  coords& operator=(const coords& c)
    { cx = c.cx; cy = c.cy; cz = c.cz; return *this; }
  coords operator*(const coords& c) const // cross product
    { return coords(cy * c.cz - cz * c.cy,
		    cz * c.cx - cx * c.cz,
		    cx * c.cy - cy * c.cx); }
  double operator/(const coords& c) // dot product
    { return cx * c.cx + cy * c.cy + cz + c.cz; }
  coords operator*(const tmatrix& m) const; // apply matrix
  coords multiply(const coords& c) const
    { return coords(cx * c.cx, cy * c.cy, cz * c.cz); }
  coords& operator*=(const tmatrix& m)
    { return *this = *this * m; }
  coords& operator*=(double s) // scaling
    { cx *= s; cy *= s; cz *= s; return *this; }
  coords& operator/=(double s) { return *this *= 1/s; }
  static const coords null;
};

inline distance::distance(coords&d) { dat = d.sqrt(); }

// some classes for doing 3D maths

// this transformation matrix is really a camera matrix,
// but that's all we need in this app anyway
// (if you need more, you are probably drawing rotating planet surfaces
//  or something and should look into using OpenGL instead...
//  of course, I wouldn't mind having OpenGL features in this app...)
class tmatrix {
 protected:
  double r[3][3]; // rotation
  double t[3];    // translation
 public:
  tmatrix(void) // unity matrix
  {
    unsigned i, j;
    for (i=0; i<3; i++) {
      for (j=0; j<3; j++) r[i][j]=0;
      r[i][i]=1;
      t[i]=0;
    }
  }

  // construct matrix from viewing angles
  // used for camera orientation in 3D space
  tmatrix(angle pitch, angle yaw, angle roll, const coords &off = coords::null, bool flip = false)
  {
    double s[3], c[3];
    s[0] = sin(pitch); c[0] = cos(pitch);
    s[1] = sin(yaw);   c[1] = cos(yaw);
    s[2] = sin(roll);  c[2] = cos(roll);

    off.get(t[0], t[1], t[2]);

    if (flip) {
      s[0] = -s[0];
      c[0] = -c[0];
      t[2] = -t[2];
    }

    // rotation order for view matrix: yaw, pitch, roll
    // didn't have a mathbook handy, so I solved it myself:
    // apply yaw (rot around y)
    // x1 = x0cos(y) - z0sin(y)
    // y1 = y0
    // z1 = x0sin(y) + z0cos(y)
    // apply pitch (rot around x)
    // x2 = x1
    //    = x0cos(y) - z0sin(y)
    // y2 = y1cos(p) - z1sin(p)
    //    = y0cos(p) - z0sin(p)
    // z2 = y1sin(p) + z1cos(p)
    //    = y0sin(p) + (x0sin(y) + z0cos(y))cos(p)
    //    = x0sin(y)cos(p) + y0sin(p) + z0cos(y)cos(p)
    // apply roll (rot around z)
    // x3 = x2cos(r) - y2sin(r)
    //    = (x0cos(y) - z0sin(y))cos(r) - (y0cos(p) - z0sin(p))sin(r)
    //    = x0cos(y)cos(r) - z0sin(y)cos(r) - y0cos(p)sin(r) - z0sin(p)sin(r)
    //    = x0cos(y)cos(r) - y0cos(p)sin(r) - z0sin(y)cos(r) - z0sin(p)sin(r)
    // y3 = x2sin(r) + y2cos(r)
    //    = (x0cos(y) - z0sin(y))sin(r) + (y0cos(p) - z0sin(p))cos(r)
    //    = x0cos(y)sin(r) - z0sin(y)sin(r) + y0cos(p)cos(r) - z0sin(p)cos(r)
    //    = x0cos(y)sin(r) + y0cos(p)cos(r) - z0sin(y)sin(r) - z0sin(p)cos(r)
    // z3 = z2
    //    = x0sin(y)cos(p) + y0sin(p) + z0cos(y)cos(p)

    r[0][0] = c[1]*c[2]; r[0][1] = -c[0]*s[2]; r[0][2] = -s[1]*c[2] -s[0]*s[2];
    r[1][0] = c[1]*s[2]; r[1][1] =  c[0]*c[2]; r[1][2] = -s[1]*s[2] -s[0]*c[2];
    r[2][0] = s[1]*c[0]; r[2][1] =  s[0];      r[2][2] =  c[1]*c[0];
  }
  // construct matrix from unit vectors
  // used to compute the equatorial->galactic transformation matrix
  tmatrix(const coords& vx, const coords& vz, const coords& off = coords::null)
  {
    coords vy(vz * vx);
    // avoid system skew - recompute up vector, just in case
    // doesn't appear to be necessary, but who knows
    vy.normalize();
    coords vu(vx * vy);

    off.get(t[0], t[1], t[2]);
    vx.get(r[0][0], r[0][1], r[0][2]);
    vy.get(r[1][0], r[1][1], r[1][2]);
    vu.get(r[2][0], r[2][1], r[2][2]);
  }
  void invert(void)
  {
    unsigned i, j;
    // To invert the rotation matrix, we only need to transpose it.
    for (i=0; i<2; i++)
      for (j=i+1; j<3; j++) {
	double n = r[i][j];
	r[i][j] = r[j][i];
	r[j][i] = n;
      }
    // To invert the translation, we need to multiply it with the
    // inverted transform and negate the result.
    double o[3];
    for (i=0; i<3; i++)
      o[i] = t[i];
    for (i=0; i<3; i++)
      t[i] = -(o[0]*r[i][0] + o[1]*r[i][1] + o[2]*r[i][2]);
  }
  tmatrix operator!()
  {
    // reverse matrix
    tmatrix m(*this);
    m.invert();
    return m;
  }
  friend class coords;
};

inline coords coords::operator*(const tmatrix& m) const
{
  return coords(cx*m.r[0][0] + cy*m.r[0][1] + cz*m.r[0][2] + m.t[0],
		cx*m.r[1][0] + cy*m.r[1][1] + cz*m.r[1][2] + m.t[1],
		cx*m.r[2][0] + cy*m.r[2][1] + cz*m.r[2][2] + m.t[2]);
}

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

  double vmag, bvmag, ubmag, rimag; // magnitudes
  wxString type;  // spectral type

  double temp;

  wxString remarks; // remarks

  stardata(void)
    : merged(FALSE), te(FALSE) {}
  void sort_names(void);
  bool has_name(const wxString& name);

  void set_pos(const coords&pos)
    { pos.get(x, y, z); }
  coords get_pos(void)
    { return coords(x, y, z); }

  void calc_temp(void);

  // TODO: add other interesting stuff to keep track of


};

WX_DECLARE_STRING_HASH_MAP(stardata*, starnamemap);

// the user interface

WX_DECLARE_LIST(stardata, starlist);

extern starlist stars;
extern starnamemap starnames;
extern void read_gliese3(const char*fname);
extern void read_bright(const char*cname,const char*nname);

class stardesc {
 public:
  const stardata *star;
  wxString desc;
  bool prepped;
  wxPoint pos;
  wxSize siz;
  stardesc(const stardata *st, coords& ref);
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
  coords pos, refpos;
  angle pitch;
  double zoom;
  bool need_redraw, ready;

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
  void DoPaint(wxDC& dc);
  void DoRedraw(void);
  void Redraw(bool clr_desc = TRUE);
  void CreateDescs(void);
  void ClearDescs(void);
  wxSize CalcBox(wxDC& dc, wxString txt, int *tabpos = (int *)NULL);
  void ShowBox(wxDC& dc, wxString txt, wxPoint pos);

  DECLARE_EVENT_TABLE()
};
