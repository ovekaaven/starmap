#include "import.h"
#include "readbright.h"
#include "readgliese.h"
#include <boost/range/adaptor/reversed.hpp>
#include <vector>
#include <wx/log.h>

const double min_vmag = 5.0; // magnitude that maps to darkest color
const double max_vmag = -3.0; // magnitude that maps to brightest color
const float min_factor = 0.1f; // ensures stars don't get too dark to see

class starcomp {
public:
  Star *main;
  std::vector<Star*> comp;

  starcomp(): main(NULL) {}
};

WX_DECLARE_STRING_HASH_MAP(starcomp*, starnamemap);
starnamemap starnames;

static Star* check_name_conflict(Star *star, const wxString& name, int ncomp)
{
  // Check whether the name to merge is already registered elsewhere.
  // For example, the Bright Star Catalog has the name "DY Eridani"
  // as another name for "Omicron(2) Eridani". On the other hand,
  // Gliese correctly lists "DY Eridani" as a separate star.
  // In this case, assuming we've loaded Gliese first, we want to
  // prevent such a merge of the name "DY Eridani".
  starnamemap::iterator it = starnames.find(name);
  if (it == starnames.end()) return nullptr;
  starcomp *comp = it->second;
  if (ncomp > 0) {
    if (comp->comp.size() >= ncomp && comp->comp[ncomp - 1] && comp->comp[ncomp - 1] != star) {
      return comp->comp[ncomp - 1];
    }
  } else if (comp->main) {
    if (comp->main != star) {
      return comp->main;
    }
  } else {
    Star* found = nullptr;
    for (Star* c : comp->comp) {
      if (!c) continue;
      if (c == star) {
        found = nullptr;
        break;
      }
      found = c;
    }
    if (found) {
      return found;
    }
  }
  return nullptr;
}

static void register_name(Star *star, const wxString& name, int ncomp)
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

static void add_star(Star *star)
{
  if (star->is3d) {
    stars.push_back(star);
  }

  for (const auto& nit : star->names) {
    register_name(star, nit.name, star->comp);
  }
}

static void merge_names(Star *star, std::list<StarName> &dat, int comp)
{
  auto nit = dat.begin();
  while (nit != dat.end()) {
    auto cit = nit;
    nit++;
    Star* conflict = check_name_conflict(star, cit->name, comp);
    if (conflict) {
      wxLogVerbose(wxT("Star %s won't merge name %s due to conflict with %s"),
                   star->names.front().name, cit->name, conflict->names.front().name);
      continue;
    }
    // register the name anew in case the component is different
    register_name(star, cit->name, comp);
    // check whether we already have the name
    if (!star->has_name(cit->name)) {
      // nope, so merge it
      star->names.splice(star->names.end(), dat, cit);
    }
  }
  // re-sort names
  star->sort_names();
}

static Star* find_merge_candidate(Star *star, int priority = -1)
{
  bool problem = false;
  // Iterate through names in reverse order because, although we
  // like to put the human-friendly names at the top of the list,
  // they tend to be the worst at distinguishing double/triple stars.
  for (const auto& nit : boost::adaptors::reverse(star->names)) {
    if (priority != -1 && nit.priority != priority) continue;
    starnamemap::iterator it = starnames.find(nit.name);
    if (it != starnames.end()) {
      starcomp *comp = it->second;
      Star *cstar;
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
          Star* found = nullptr;
          bool multiple = false;
          for (Star* c : comp->comp) {
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
        return cstar;
      }
    }
  }
  if (problem) {
    wxLogVerbose(wxT("Merge of %s seems to have failed because of problem"), star->names.front().name);
  }
  return nullptr;
}

static bool merge_star(Star *star)
{
  // Start by trying to match relatively reliable naming systems...
  Star* cstar = find_merge_candidate(star, ReadBase::PRI_HD);
  if (!cstar) {
    // If that fails, fall back to other systems
    cstar = find_merge_candidate(star);
  }
  if (cstar) {
    // found match, merge
#if 0
    wxLogVerbose(wxT("Merging stars %s and %s because of name %s"),
                 cstar->names.front().name, star->names.front().name,
                 nit.name);
#endif
    merge_names(cstar, star->names, star->comp);
    if (!star->remarks.IsEmpty()) {
      if (!cstar->remarks.IsEmpty()) cstar->remarks += wxT(' ');
      cstar->remarks += star->remarks;
    }
    if (!cstar->comp && star->comp) {
      // merging the component might help the UI display
      // binary systems without too much overlapping text
      cstar->comp = star->comp;
    }
    if (!cstar->is3d && star->is3d) {
#if 0
      wxLogVerbose(wxT("Converting star %s to 3D"), star->names.front().name);
#endif
      cstar->is3d = star->is3d;
      cstar->pos = star->pos;
      cstar->vmag = star->vmag;
      cstar->color = star->color;
      // Not sure if it makes sense to also overwrite type and temp,
      // but we'll do it for consistency, because the type and temp
      // of the merged star is currently used for the merged color.
      // Maybe we'll want to change that later.
      cstar->type = star->type;
      cstar->temp = star->temp;
      stars.push_back(cstar);
    }
    delete star;
    return true;
  }
  // not found, consider it a new star
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
    float mag_factor = (float)((min_vmag - data.vmag) / (min_vmag - max_vmag));
    mag_factor = std::max(mag_factor, 0.0f) * (1.0f - min_factor) + min_factor;

    // Copy data to final data structure
    Star* star = new Star;
    star->is3d = data.is3d;
    star->pos = data.position;
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
    star->names.push_back(data.name);
    for (const auto& name : data.other_names) {
      star->names.push_back(name);
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
