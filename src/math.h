#pragma once
#include <cmath>
#include <algorithm>

typedef double real_t;
static const real_t PI = real_t(3.1415926535897932384626);

struct vec3_t {
  vec3_t(real_t x = 0.0f, real_t y = 0.0f, real_t z = 0.0f)
      : x(x), y(y), z(z) {}
  vec3_t& operator=(vec3_t const& v) {
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
  }
  vec3_t operator-() const { return vec3_t(-x, -y, -z); }

  real_t x, y, z;
};

inline vec3_t operator+(vec3_t const &a, vec3_t const &b) {
  return vec3_t(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline vec3_t operator-(vec3_t const &a, vec3_t const &b) {
  return vec3_t(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline vec3_t operator*(vec3_t const &a, vec3_t const &b) {
  return vec3_t(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline vec3_t operator*(vec3_t const &a, real_t r) {
  return vec3_t(a.x * r, a.y * r, a.z * r);
}

inline vec3_t operator*(real_t r, vec3_t const &a) {
  return vec3_t(a.x * r, a.y * r, a.z * r);
}

inline vec3_t &operator*=(vec3_t &v, float f) {
  v.x *= f;
  v.y *= f;
  v.z *= f;
  return v;
}

inline vec3_t &operator+=(vec3_t &a, vec3_t const &b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

inline real_t dot(vec3_t const &a, vec3_t const &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3_t cross(vec3_t const &a, vec3_t const &b) {
  return vec3_t(a.y * b.z - b.y * a.z,
                a.z * b.x - b.z * a.x,
                a.x * b.y - a.y * b.x);
}

inline real_t clamp(real_t const& v, real_t min, real_t max) {
  return std::max(std::min(max, v), min);
}

inline vec3_t clamp(vec3_t const& v, real_t min, real_t max) {
  return vec3_t(
    clamp(v.x, min, max),
    clamp(v.y, min, max),
    clamp(v.z, min, max)
  );
}

inline vec3_t clamp(vec3_t const& v, vec3_t const& min, vec3_t const& max) {
  return vec3_t(
    clamp(v.x, min.x, max.x),
    clamp(v.y, min.y, max.y),
    clamp(v.z, min.z, max.z)
  );
}

inline real_t lengthSquare(vec3_t const &a) { return dot(a, a); }

inline real_t length(vec3_t const &a) { return std::sqrt(lengthSquare(a)); }

inline vec3_t normalize(vec3_t const &a) { return real_t(1.0) / length(a) * a; }
