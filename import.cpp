#include "import.h"
#include "readbright.h"
#include "readgliese.h"
#include "starmap.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(starlist);

const double min_vmag = 5.0; // magnitude that maps to darkest color
const double max_vmag = -3.0; // magnitude that maps to brightest color
const float min_factor = 0.1f; // ensures stars don't get too dark to see

starlist stars;
starnamemap starnames;

void stardata::sort_names()
{
  names.sort();
}

bool stardata::has_name(const wxString& name)
{
  for (const auto &nit : names) {
    if (nit.name == name) return true;
  }
  return false;
}

starname to_starname(const ReadBase::StarName& name) {
  return starname(name.name, name.priority);
}

static void register_name(stardata *star, const wxString& name, int ncomp)
{
  starnamemap::iterator it = starnames.find(name);
  starcomp *comp;
  if (it != starnames.end()) {
    comp = it->second;
  } else {
    comp = new starcomp;
    starnames[name] = comp;
  }
  if (ncomp > 0) {
    if (comp->comp.size() < ncomp) {
      comp->comp.resize(ncomp);
    }
    comp->comp[ncomp - 1] = star;
  } else {
    comp->main = star;
  }
}

static void add_star(stardata *star)
{
  stars.push_back(star);

  for (const auto& nit : star->names) {
    register_name(star, nit.name, star->comp);
  }
}

static void merge_names(stardata *star, std::list<starname> &dat, int comp)
{
  auto nit = dat.begin();
  while (nit != dat.end()) {
    // register the name anew in case the component is different
    register_name(star, nit->name, comp);
    auto cit = nit;
    nit++;
    // check whether we already have the name
    if (!star->has_name(cit->name)) {
      // nope, so merge it
      star->names.splice(star->names.end(), dat, cit);
    }
  }
  // re-sort names
  star->sort_names();
}

static bool merge_star(stardata *star)
{
  bool problem = false;
  for (const auto& nit : star->names) {
    starnamemap::iterator it = starnames.find(nit.name);
    if (it != starnames.end()) {
      starcomp *comp = it->second;
      stardata *cstar;
      // in several common naming systems, the component stars of a binary star system
      // don't necessarily have distinct names, so grab the right component before merging
      if (star->comp > 0) {
        if (star->comp <= comp->comp.size()) {
          cstar = comp->comp[star->comp - 1];
        } else {
          cstar = NULL;
        }
        if (!cstar && comp->main) {
          if (comp->comp.empty()) {
            // Seems this name was registered without components, so there shouldn't
            // be much risk of ambiguity if we merge with it.
            cstar = comp->main;
          } else {
            // Note that it is *not* safe to merge if the name was registered with
            // components, even if all existing registrations point to the exact
            // same star as comp->main. After all, maybe we're adding a second
            // component to an existing name.
#if 0
            wxLogVerbose(wxT("Potential merge problem: star %s has a component, "
                             "but name %s is registered both with and without components"),
                         star->names.front().name, nit.name);
            problem = true;
#endif
          }
        }
      } else {
        cstar = comp->main;
        if (!cstar && !comp->comp.empty()) {
          // Seems this name was registered with components. If this is a naming
          // system that has distinct names for components, we should only find
          // one component. If so, it should be safe enough to merge with it.
          stardata* found = nullptr;
          bool multiple = false;
          for (stardata* c : comp->comp) {
            if (!c) continue;
            if (!found) found = c;
            else multiple = true;
          }
          if (!multiple) {
            cstar = found;
          } else {
#if 0
            wxLogVerbose(wxT("Potential merge problem: star %s has no component, "
                             "but name %s is registered with multiple components"),
                         star->names.front().name, nit.name);
            problem = true;
#endif
          }
        }
      }
      if (cstar) {
        // found match, merge
#if 0
        wxLogVerbose(wxT("Merging stars %s and %s because of name %s"),
                     cstar->names.front().name, star->names.front().name,
                     nit.name);
#endif
        merge_names(cstar, star->names, star->comp);
        if (!cstar->comp && star->comp) {
          // merging the component might help the UI display
          // binary systems without too much overlapping text
          cstar->comp = star->comp;
        }
        cstar->merged = TRUE;
        delete star;
        return true;
      }
    }
  }
  if (problem) {
    wxLogVerbose(wxT("Merge of %s seems to have failed because of problem"), star->names.front().name);
  }
  // not found, consider it a new star
  star->merged = TRUE;
  add_star(star);
  return false;
}

void import_catalog(ReadBase& importer) {
  if (!importer.IsOk()) {
    return;
  }

  wxLogVerbose(wxT("Loading %s..."), importer.GetCatalogName());

  ReadBase::StarData data;
  while (importer.ReadNext(data)) {
    if (!data.is3d) continue;

    float mag_factor = (float)((min_vmag - data.vmag) / (min_vmag - max_vmag));
    mag_factor = std::max(mag_factor, 0.0f) * (1.0f - min_factor) + min_factor;

    // Copy data to final data structure
    stardata* star = new stardata;
    data.position.get(star->x, star->y, star->z);
    star->vmag = data.vmag;
    star->type = data.spectral_type;
    star->temp = data.temperature;
    star->color = (data.color * mag_factor).ToDisplay();
    star->remarks = data.remarks;

    if (!data.components.IsEmpty()) {
      wxChar comp = data.components[0];
      star->comp = (comp >= wxT('A')) ? (comp - wxT('A') + 1) : 0;
    } else {
      star->comp = 0;
    }
    star->names.push_back(to_starname(data.name));
    for (const auto& name : data.other_names) {
      star->names.push_back(to_starname(name));
    }
    star->sort_names();

    merge_star(star);
  }
}

void import_all() {
  {
    ReadGliese catalog(wxT("gliese"));
    import_catalog(catalog);
  }
  {
    ReadBright catalog(wxT("bright"));
    import_catalog(catalog);
  }
  wxLogVerbose(wxT("Loaded %zu stars."), stars.size());
}
