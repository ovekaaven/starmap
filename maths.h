#ifndef STARMAP_MATHS_H
#define STARMAP_MATHS_H

#include <cmath>
#include <wx/gdicmn.h>

class Angle
{
protected:
  double _rad;
public:
  Angle() { _rad = 0.0; }
  explicit Angle(double r) { _rad = r; } // angle (radians)

  double rad() const { return _rad; }

  double sin() const { return ::sin(_rad); }
  double cos() const { return ::cos(_rad); }

  double operator+=(double d) { return _rad += d; }
  double operator-=(double d) { return _rad -= d; }

  static Angle from_deg(double d) { return Angle(d * M_PI / 180.0); }
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
  double get_x() const { return _x; }
  double get_y() const { return _y; }
  double get_z() const { return _z; }

  void flatten() { _z = 0.0; }
  bool behind() const { return _z < 0.1; }
  double depth() const { return _z; }

  // perspective projection
  wxPoint pproject(double s, int xc, int yc) const {
    return wxPoint(s * _x / _z + xc, s * _y / _z + yc);
  }

  double sqr() const {
    return _x*_x + _y*_y + _z*_z;
  }
  double norm() const {
    return ::sqrt(sqr());
  }
  void normalize() {
    *this /= norm();
  }

  Vector multiply(const Vector& v) const {
    return Vector(_x * v._x, _y * v._y, _z * v._z);
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

  // construct matrix from Euler angles
  // used for camera orientation in 3D space
  Transform(Angle pitch, Angle yaw, Angle roll, const Vector &off = Vector::null, bool flip = false)
  {
    double s[3], c[3];
    s[0] = pitch.sin(); c[0] = pitch.cos();
    s[1] = yaw.sin();   c[1] = yaw.cos();
    s[2] = roll.sin();  c[2] = roll.cos();

    off.get(_t[0], _t[1], _t[2]);

    if (flip) {
      s[0] = -s[0];
      c[0] = -c[0];
      _t[1] = -_t[1];
    }

    // rotation order for view matrix: yaw, pitch, roll
    _r[0][0] = c[1]*c[2]; _r[0][1] = -c[0]*s[2]; _r[0][2] = -s[1]*c[2] -s[0]*s[2];
    _r[1][0] = c[1]*s[2]; _r[1][1] =  c[0]*c[2]; _r[1][2] = -s[1]*s[2] -s[0]*c[2];
    _r[2][0] = s[1]*c[0]; _r[2][1] =  s[0];      _r[2][2] =  c[1]*c[0];
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
