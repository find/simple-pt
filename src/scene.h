#pragma once
#include "geometry.h"
#include "material.h"
#include <vector>
#include <string>
#include <unordered_map>

struct camera_t {
  vec3_t position;
  vec3_t direction;
  vec3_t up;
  real_t fov;
  real_t near;
  real_t far;
};

class Scene {
public:
  Scene() = default;

  std::vector<Geometry*>   geometry_list;
  std::unordered_map<std::string, material_t> material_list;
  camera_t camera;
  ~Scene() {
    for(Geometry* g : geometry_list) {
      // delete g;
      g->~Geometry();
    }
  }

  bool read(std::string const& fliename);
};

