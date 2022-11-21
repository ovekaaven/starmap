#include "maths.h"

const Vector Vector::null;

Vector Vector::operator*(const Transform& t) const {
  return Vector(_x*t._r[0][0] + _y*t._r[0][1] + _z*t._r[0][2] + t._t[0],
                _x*t._r[1][0] + _y*t._r[1][1] + _z*t._r[1][2] + t._t[1],
                _x*t._r[2][0] + _y*t._r[2][1] + _z*t._r[2][2] + t._t[2]);
}
