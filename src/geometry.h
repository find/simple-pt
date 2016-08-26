#pragma once
#include "math.h"
#include "material.h"

struct ray_t {
  vec3_t origin;
  vec3_t direction;
};

struct intersection_t {
  vec3_t position;
  vec3_t normal;
  material_t const *material;
};

class Geometry {
public:
  virtual ~Geometry() {}
  virtual bool intersect(ray_t const &ray,
                         intersection_t *intersection) const = 0;

  material_t material;
};

struct Sphere : public Geometry {
public:
  virtual bool intersect(ray_t const &ray,
                         intersection_t *intersection) const override;

  vec3_t center;
  real_t radius;
};

class Plane : public Geometry {
public:
  virtual bool intersect(ray_t const &ray,
                         intersection_t *intersection) const override;

  vec3_t center;
  vec3_t normal;
};

class Disk : public Plane {
public:
  virtual bool intersect(ray_t const &ray,
                         intersection_t *intersection) const override;

  real_t radius;
};

class OrientedBox : public Geometry {
public:
  virtual bool intersect(ray_t const &ray,
                         intersection_t *intersection) const override;

  vec3_t center;
  vec3_t axis[3];
  vec3_t extent;
};


