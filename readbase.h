#ifndef STARMAP_READBASE_H
#define STARMAP_READBASE_H

#include "colors.h"
#include "maths.h"
#include "starlist.h"

#include <list>
#include <boost/iostreams/filtering_stream.hpp>
#include <wx/colour.h>
#include <wx/filename.h>
#include <wx/string.h>

class ReadBase {
public:

  // this priority ordering is more or less arbitrary
  // so just reprioritize as you see fit
  enum priority {
    PRI_Common    = 0,  // Sirius, Procyon
    PRI_Bayer     = 1,  // Alpha Centauri
    PRI_Variable  = 2,  // UV Ceti
    PRI_Flamsteed = 3,  // 44 Iota Bootis
    PRI_Simple    = 4,  // Ross 128, Wolf 359
    PRI_Other     = 5,
    PRI_DM        = 6,  // BD+36°2147 (Durchmusterung)
    PRI_AC        = 7,  // AC+82:1111 (Astrographic Catalogue)
    PRI_Giclas    = 8,  // G 267-25

    PRI_Luyten    = 10, // L 1154-029
    PRI_BPM       = 11, // BPM 46239 (Luyten: Bruce Proper Motion Survey)
    PRI_LFT       = 12, // LFT 94 (Luyten Five-Tenth Catalogue)
    PRI_LHS       = 13, // LHS 102 (Luyten Half-Second Catalogue)
    PRI_LPM       = 14, // LP 462-42 (Luyten Proper-Motion Catalogue)
    PRI_LTT       = 15, // LTT 20 (Luyten Two-Tenth Catalogue)

    PRI_Gliese    = 16,

    PRI_HD        = 20, // Henry Draper
    PRI_ADS       = 30, // Aitken Double Star
    PRI_Vyssotsky = 40,
    PRI_SAO       = 45, // Smithsonian Astrophysical Observatory
    PRI_FK5       = 50,
    PRI_Harvard   = 51,
    PRI_UGPMF     = 52,
    PRI_EGGR      = 53,

    PRI_Hipparcos = 60,
  };

  struct StarData {
    bool is3d = false;

    // If is3d is true: position vector in rectangular galactic coordinates. Units in parsecs.
    // If is3d is false: direction vector to star's apparent position.
    Vector position;

    // Epoch of observation, i.e., the year for which this position applies.
    // In theory, the motion vector could be used to calculate the expected position
    // at any other epoch, within reason.
    double epoch;

    // Rate of change of the position/direction vector, in units per year.
    Vector motion;

    // If is3d is true: absolute magnitude.
    // If is3d is false: apparent magnitude.
    double vmag = 0.0;

    // Harvard or Yerkes spectral classification.
    wxString spectral_type;

    // Effective (i.e. photospheric) temperature, in Kelvin.
    // Determines the star's apparent color.
    double temperature = 0.0;

    // Visible color, ignoring magnitude.
    Color color;

    // Catalog's primary designation.
    StarName name;

    // Components covered by this catalog record.
    wxString components;

    // Other known designations.
    std::list<StarName> other_names;

    // Additional notes of interest.
    wxString remarks;

    void SetName(const wxString& n, int p) {
      name = StarName(n, p);
    }

    void ClearLists() {
      other_names.clear();
      remarks.Clear();
    }
    void AddName(const wxString& n, int p) {
      other_names.emplace_back(StarName(n, p));
    }
    void AddRemark(const wxString& remark) {
      if (!remarks.IsEmpty()) remarks += wxT(' ');
      remarks += remark;
    }
  };

  virtual bool IsOk() { return true; }
  virtual wxString GetCatalogName() = 0;
  virtual bool ReadNext(StarData& data) = 0;

protected:

  struct WorkData {
    StarData* star;

    double ra = 0.0; // right ascension in radians
    double de = 0.0; // declination in radians
    double plx = 0.0; // parallax in milliarcsec
    double pmra = 0.0; // RA proper motion * cos(DE), milliarcsec/year
    double pmde = 0.0; // DE proper motion, milliarcsec/year
    double rvel = 0.0; // Radial velocity, km/s
    double vmag = 0.0; // Apparent visual magnitude
    double bvmag = 0.0; // Johnson B-V color index

    explicit WorkData(StarData& data): star(&data) {}
  };

  static bool OpenStream(boost::iostreams::filtering_istream& stream, const wxFileName& name);

  static void ReadRA(WorkData& data, const std::string& h, const std::string& m, const std::string& s);
  static void ReadDE(WorkData& data, char sg, const std::string& d, const std::string& m, const std::string& s);
  static void ReadDE(WorkData& data, char sg, const std::string& d, const std::string& m);
  static void ReadSpectralType(StarData& data, const std::string& type);
  static bool ReadComponents(StarData& data, const std::string& comps);
  static bool ReadDurchmusterung(StarData& data, const std::string& cat,
                                 const std::string& dec, const std::string& num);
  static bool ReadDurchmusterung(StarData& data, const std::string& cat,
                                 const std::string& id);
  static bool ReadDurchmusterung(StarData& data, const std::string& id);
  static bool ReadGiclas(StarData& data, const std::string& id);
  static bool ReadOtherName(StarData& data, const wxString& pfx, const std::string& name, int priority);

  static bool LookupConstellation(wxString& name, const std::string& tok);

  static wxString MakeSuperscript(const std::string& num);

  static const Transform B1950;
  static const Transform J2000;

  static void Calculate(WorkData& data, const Transform& frame, double epoch);

  static double EstimateTemperature(const wxString& type, double bvmag);
  static double EstimateTemperatureFromBV(double bvmag);
  static double CombineWithTemperatureFromBV(double temp, double bvmag, bool bad_subdiv);
};

#endif //STARMAP_READBASE_H
