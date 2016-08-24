#pragma once
#include "math.h"

struct material_t {
  vec3_t color;
  real_t roughness;
  bool   emit;
};

