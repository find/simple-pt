#include "geometry.h"
#include <limits>
#include <algorithm>

/// reference: http://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool Sphere::intersect(ray_t const& ray, intersection_t *intersection) const {
  if (!intersection) {
    return false;
  }

  vec3_t diff = ray.origin - center;
  real_t a0 = lengthSquare(diff) - radius*radius;
  real_t a1 = dot(ray.direction, diff);
  real_t discr = a1*a1-a0;
  if (discr < real_t(0)) {
    intersection->num = 0;
    return false;
  }

  real_t t[2] = { real_t(0), real_t(0) };

  if (std::fabs(a0) < 1e-5) { // on the surface
    intersection->num = 0;
    return false;
  }
  if (a0 < real_t(0)) {
    // ray starts inside the sphere
    intersection->num = 1;
    real_t root = std::sqrt(discr);
    t[0] = -a1 + root;
  } else if (discr > real_t(0)) {
    intersection->num = 2;
    real_t root = std::sqrt(discr);
    t[0] = -a1 - root;
    t[1] = -a1 + root;
  } else { // discr == 0
    intersection->num = 1;
    t[0] = -a1;
  }

  if (intersection->num > 0) {
    if (t[0] < 0 && intersection->num > 1) {
      --intersection->num;
      t[0] = t[1];
    }
    if (t[0] < 0) {
      intersection->num = 0;
      return false;
    }
    intersection->intersection[0] = ray.origin + ray.direction * t[0];
    intersection->normal[0] = normalize(intersection->intersection[0] - center);
    if (intersection->num > 1) {
      intersection->intersection[1] = ray.origin + ray.direction * t[1];
      intersection->normal[1] = normalize(intersection->intersection[1] - center);
    }
    intersection->material = &material;
    return true;
  } else {
    return false;
  }
}

/// reference: http://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
bool Plane::intersect(ray_t const& ray, intersection_t *intersection) const {
  if (!intersection) {
    return false;
  }
  real_t denom = dot(normal, ray.direction);
  real_t t = real_t(0);
  if (std::abs(denom) > real_t(1.0e-6)) {
    real_t const dist_to_origin = dot(normal, center);
    real_t const dist_to_ray_origin = dot(normal, ray.origin) - dist_to_origin;
    t = -dist_to_ray_origin / denom;
  }
  if (t <= 1e-5) {
    intersection->num = 0;
    return false;
  }
  vec3_t p = ray.origin + ray.direction*t;

  intersection->num = 1;
  intersection->intersection[0] = p;
  intersection->normal[0] = normal;
  intersection->material = &material;
  return true;
}

bool Disk::intersect(ray_t const &ray, intersection_t *intersection) const {
  // intersect plane:
  if (!this->Plane::intersect(ray, intersection)) {
    return false;
  }
  vec3_t d = intersection->intersection[0] - center;
  if (dot(d,d) > radius*radius) {
    intersection->num = 0;
    return false;
  }
  return true;
}

// reference: GTEngine
static bool clip(real_t denom, real_t numer, vec3_t const& n0, vec3_t const& n1,
                 real_t *t0, real_t *t1, vec3_t *on0, vec3_t *on1) {
  if (denom > real_t(0)) {
    if (numer > denom * *t1) {
      return false;
    }
    if (numer > denom * *t0) {
      *t0 = numer / denom;
      *on0 = n0;
    }
    return true;
  } else if (denom < real_t(0)) {
    if (numer > denom * *t0) {
      return false;
    }
    if (numer > denom * *t1) {
      *t1 = numer / denom;
      *on1 = n1;
    }
    return true;
  } else {
    return numer <= real_t(0);
  }
}

/// reference: GTEngine
/// reference: http://www.opengl-tutorial.org/cn/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
bool OrientedBox::intersect(ray_t const& ray, intersection_t *intersection) const {
  // transform line into oriented-box coordinate system
  vec3_t const diff = ray.origin - center;
  vec3_t const origin = vec3_t( dot(diff, axis[0]),
                                dot(diff, axis[1]),
                                dot(diff, axis[2]) );
  vec3_t const direction = vec3_t( dot(ray.direction, axis[0]),
                                   dot(ray.direction, axis[1]),
                                   dot(ray.direction, axis[2]) );

  if (std::abs(std::abs(origin.x) - extent.x) < 1e-5 ||
      std::abs(std::abs(origin.y) - extent.y) < 1e-5 ||
      std::abs(std::abs(origin.z) - extent.z) < 1e-5) { // ray starts at surface of this
    intersection->num = 0;
    return false;
  }
  real_t t0 = real_t(0);
  real_t t1 = std::numeric_limits<real_t>::max();
  vec3_t n0, n1;
  bool   lineIntersect = false;
  if (clip( direction.x, -origin.x - extent.x, -axis[0], axis[0], &t0, &t1, &n0, &n1) &&
      clip(-direction.x,  origin.x - extent.x, axis[0], -axis[0], &t0, &t1, &n0, &n1) &&
      clip( direction.y, -origin.y - extent.y, -axis[1], axis[1], &t0, &t1, &n0, &n1) &&
      clip(-direction.y,  origin.y - extent.y, axis[1], -axis[1], &t0, &t1, &n0, &n1) &&
      clip( direction.z, -origin.z - extent.z, -axis[2], axis[2], &t0, &t1, &n0, &n1) &&
      clip(-direction.z,  origin.z - extent.z, axis[2], -axis[2], &t0, &t1, &n0, &n1) ) {
    lineIntersect = true;
  } else {
    intersection->num = 0;
    return false;
  }

  if (t1 < 0) {
    intersection->num = 0;
    return false;
  }
  if (t0 < 0 || t0 == t1) { // t1 >=0
    intersection->num = 1;
    intersection->intersection[0] = ray.origin + t1 * ray.direction;
    intersection->normal[0] = n1;
  } else { // t0 >=0 && t1 >= 0
    intersection->num = 2;
    intersection->intersection[0] = ray.origin + t0 * ray.direction;
    intersection->intersection[1] = ray.origin + t1 * ray.direction;
    intersection->normal[0] = n0;
    intersection->normal[1] = n1;
  }
  intersection->material = &material;
  return true;
}

