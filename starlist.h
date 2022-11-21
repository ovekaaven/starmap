#ifndef STARMAP_STARLIST_H
#define STARMAP_STARLIST_H

#include "maths.h"
#include <list>
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

struct StarName {
  wxString name;
  int priority = 0;
  StarName() = default;
  StarName(const wxString& n, int p) : name(n), priority(p) {}
  // StarName(const StarName& other) : name(other.name), priority(other.priority) {}
  bool operator<(const StarName& other) const { return priority < other.priority; }
  bool operator>(const StarName& other) const { return priority > other.priority; }
};

class Star {
public:
  bool is3d;
  std::list<StarName> names;
  int comp;

  Vector pos;     // star coordinates (parsecs, heliocentric)
  wxPoint proj;   // current projection point
  bool show;      // current visibility
  wxCoord tw, th; // text extents
  bool te;        // text extents

  double vmag;    // visual magnitude
  wxString type;  // spectral type
  wxColour color;

  double temp;

  wxString remarks; // remarks

  Star(): te(FALSE) {}
  void sort_names();
  bool has_name(const wxString& name);

  const Vector& get_pos() const { return pos; }
};

extern std::list<Star*> stars;

#endif //STARMAP_STARLIST_H
