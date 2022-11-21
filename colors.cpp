#include "colors.h"

#include <cmath>
#include <wx/log.h>

float Color::ApplyGamma(float c) {
  return c < 0.0031308 ? (12.92f * c) : (1.055f * powf(c, 1.0f / 2.4f) - 0.055f);
}

unsigned char Color::To8bit(float c) {
  return c > 1.0f ? 255 : c < 0.0f ? 0 : (unsigned)(c * 255.0f);
}

// CIE 1931 XYZ to linear sRGB
Color Color::FromXYZ(float x, float y, float z) {
  return Color( 3.2406f * x + -1.5372f * y + -0.4986f * z,
                -0.9689f * x +  1.8758f * y +  0.0415f * z,
                0.0557f * x + -0.2040f * y +  1.0570f * z);
}

Color Color::FromTemperature(double temperature) {
  // Approximation of Planckian locus in CIE 1960 UCS space
  double temperature2 = temperature * temperature;
  double u =
      (0.860117757 + 1.5411825e-4 * temperature + 1.28641212e-7 * temperature2) /
      (1.0 + 8.42420235e-4 * temperature + 7.08145163e-7 * temperature2);
  double v =
      (0.317398726 + 4.22806245e-5 * temperature + 4.20481691e-8 * temperature2) /
      (1.0 - 2.89741816e-5 * temperature + 1.61456053e-7 * temperature2);

  // Convert to CIE 1931 XYZ space
  double x = 3.0 * u / (2.0 * u - 8.0 * v + 4.0);
  double y = 2.0 * v / (2.0 * u - 8.0 * v + 4.0);
  double z = 1.0 - x - y;

  float Y = 1.0f;
  float X = (float)(Y / y * x);
  float Z = (float)(Y / y * z);

  return FromXYZ(X, Y, Z);
}

wxColour Color::ToDisplay() const {
  float r = ApplyGamma(_r);
  float g = ApplyGamma(_g);
  float b = ApplyGamma(_b);
  float m = std::max(std::max(r, g), b);
  if (m > 1.0f) {
    r /= m;
    g /= m;
    b /= m;
  }
  return wxColour(To8bit(r), To8bit(g), To8bit(b));
}

SpectralType::SpectralType(const wxString& type) {
  size_t pos = 0;

  // Extract Mount Wilson class, if present
  while (wxIslower(type[pos])) {
    _mw += type[pos++];
  }
  if (type[pos] == wxT(':')) pos++;

  // Get class
  while (wxIsupper(type[pos])) {
    _cls += type[pos++];
  }

  // Get subdivision
  _sub = 0.0f;
  wxChar dig = type[pos++];
  if (wxIsdigit(dig)) {
    _sub = dig - wxT('0');
    // Subdivisions may be fractional. We'll handle one decimal.
    if (type[pos] == wxT('.') && wxIsdigit(type[pos+1])) {
      _sub += (type[pos+1] - wxT('0')) * 0.1;
      pos += 2;
    }
  } else {
    // wxLogVerbose(wxT("Strange spectral class %s"), type);
    _sub = NAN;
  }

  // Get main part of luminosity class
  while (type[pos] == wxT('I')) {
    _lum += type[pos++];
  }
  if (type[pos] == wxT('V')) {
    _lum += type[pos++];
    while (type[pos] == wxT('I')) {
      _lum += type[pos++];
    }
  }
  if (_lum.IsEmpty() && type[pos] == wxT('0')) {
    _lum += type[pos++];
  }
  if (type[pos] == wxT(':')) pos++;

  _pec = type.Mid(pos);

  if (_cls.IsEmpty() && std::isnan(_sub) && _lum.IsEmpty()) {
    // If the spectral type is simply something like "pec",
    // let's put it into _pec instead of _mw.
    _pec = _mw + _pec;
    _mw.Clear();
  }
}
