#ifndef STARMAP_MATHS_H
#define STARMAP_MATHS_H

#include <cmath>

class Angle
{
protected:
  double _rad;
public:
  explicit Angle(double r) { _rad = r; } // angle (radians)

  double rad() const { return _rad; }

  double sin() const { return ::sin(_rad); }
  double cos() const { return ::cos(_rad); }

  static Angle from_deg(double d) {
    return Angle(d * M_PI / 180.0); }
};

class Transform;

class Vector
{
protected:
  double _x, _y, _z;
public:
  Vector(): _x(0.0), _y(0.0), _z(0.0) {}
  Vector(double x, double y, double z): _x(x), _y(y), _z(z) {}

  // cross product
  Vector(const Vector& a, const Vector& b) {
    _x = a._y * b._z - a._z * b._y;
    _y = a._z * b._x - a._x * b._z;
    _z = a._x * b._y - a._y * b._x;
  }

  void get(double& x, double& y, double& z) const {
    x = _x;
    y = _y;
    z = _z;
  }

  double sqr() const {
    return _x*_x + _y*_y + _z*_z;
  }
  double sqrt() const {
    return ::sqrt(sqr());
  }
  void normalize() {
    *this /= sqrt();
  }

  Vector operator+(const Vector& v) const {
    return Vector(_x + v._x,
                  _y + v._y,
                  _z + v._z);
  }
  Vector operator-(const Vector& v) const {
    return Vector(_x - v._x,
                  _y - v._y,
                  _z - v._z);
  }
  Vector operator*(double s) const {
    return Vector(_x * s,
                  _y * s,
                  _z * s);
  }
  Vector operator/(double s) const {
    return Vector(_x / s,
                  _y / s,
                  _z / s);
  }

  Vector& operator+=(const Vector& v)
  {
    _x += v._x;
    _y += v._y;
    _z += v._z;
    return *this;
  }
  Vector& operator-=(const Vector& v)
  {
    _x -= v._x;
    _y -= v._y;
    _z -= v._z;
    return *this;
  }
  Vector& operator*=(double s)
  {
    _x *= s;
    _y *= s;
    _z *= s;
    return *this;
  }
  Vector& operator/=(double s) {
    _x /= s;
    _y /= s;
    _z /= s;
    return *this;
  }

  Vector operator*(const Transform& t) const;

  // direction vector
  static Vector dir(Angle phi, Angle theta) {
    double rvect = theta.cos();
    return Vector(rvect * phi.cos(),
                  rvect * phi.sin(),
                  theta.sin());
  }

  // partial derivative with respect to phi
  static Vector d_phi(Angle phi, Angle theta) {
    double rvect = theta.cos();
    return Vector(rvect * -phi.sin(),
                  rvect * phi.cos(),
                  0.0);
  }

  // similar to above, but assume the caller has
  // its own scaling factor to handle cos(theta)
  static Vector d_phi_s(Angle phi) {
    return Vector(-phi.sin(),
                  phi.cos(),
                  0.0);
  }

  // partial derivative with respect to theta
  static Vector d_theta(Angle phi, Angle theta) {
    double rvect = -theta.sin();
    return Vector(rvect * phi.cos(),
                  rvect * phi.sin(),
                  theta.cos());
  }

  static const Vector null;
};

class Transform {
protected:
  double _r[3][3]; // rotation
  double _t[3];    // translation
  friend class Vector;
public:
  // identity transform
  Transform()
  {
    unsigned i, j;
    for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
        _r[i][j] = 0.0;
      }
      _r[i][i] = 1.0;
      _t[i] = 0.0;
    }
  }

  // construct matrix from unit vectors
  // used to compute the equatorial->galactic transformation matrix
  Transform(const Vector& vx, const Vector& vz, const Vector& off = Vector::null)
  {
    Vector vy(vz, vx);
    // avoid system skew - recompute up vector, just in case
    vy.normalize();
    Vector vu(vx, vy);

    off.get(_t[0], _t[1], _t[2]);
    vx.get(_r[0][0], _r[0][1], _r[0][2]);
    vy.get(_r[1][0], _r[1][1], _r[1][2]);
    vu.get(_r[2][0], _r[2][1], _r[2][2]);
  }
};

#endif //STARMAP_MATHS_H
