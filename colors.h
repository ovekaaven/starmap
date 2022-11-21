#ifndef STARMAP_COLORS_H
#define STARMAP_COLORS_H

#include <wx/colour.h>

class Color {
protected:
  float _r, _g, _b;

  static float ApplyGamma(float c);
  static unsigned char To8bit(float c);
public:
  Color(): _r(0.0), _g(0.0), _b(0.0) {}
  Color(float r, float g, float b): _r(r), _g(g), _b(b) {}

  void get(float& r, float& g, float& b) const {
    r = _r;
    g = _g;
    b = _b;
  }

  Color operator*(float s) const {
    return Color(_r * s,
                 _g * s,
                 _b * s);
  }

  static Color FromXYZ(float x, float y, float z);
  static Color FromTemperature(double temperature);

  // Get gamma-compressed sRGB value
  wxColour ToDisplay() const;
};

class SpectralType {
protected:
  wxString _mw;
  wxString _cls;
  float _sub;
  wxString _lum;
  wxString _pec;

public:
  SpectralType(): _sub(0.0f) {}
  SpectralType(const wxString& type);

  const wxString& GetClass() const { return _cls; }
  float GetSubdivision() const { return _sub; }
  const wxString& GetPeculiarity() const { return _pec; }

  bool IsSupergiant() const {
    return (_lum == wxT("0") || _lum == wxT("I") || _mw == wxT("c"));
  }
  bool IsGiant() const {
    return (_lum == wxT("II") || _lum == wxT("III") || _mw == wxT("g"));
  }
};

#endif //STARMAP_COLORS_H
