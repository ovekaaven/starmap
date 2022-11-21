#include "starlist.h"

std::list<Star*> stars;

void Star::sort_names()
{
  names.sort();
}

bool Star::has_name(const wxString& name)
{
  for (const auto& nit : names) {
    if (nit.name == name) return true;
  }
  return false;
}
