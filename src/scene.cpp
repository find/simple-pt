#include "scene.h"
#include "../3rdparty/pugixml/pugixml.hpp"

#include <stdio.h>
#include <assert.h>
#include <string.h>

class MemPool {
  uint8_t *mem_;
  size_t   size_;
  size_t   head_;

public:
  MemPool(size_t size) :size_(size), head_(0) {
    mem_ = static_cast<uint8_t*>(_aligned_malloc(size,16));
  }
  ~MemPool() {
    _aligned_free(mem_);
  }
  void reset() {
    head_ = 0;
  }
  void* alloc(size_t size) {
    head_ += size;
    assert(head_ <= size_);
    return mem_ + (head_ - size);
  }
};

void* operator new(size_t sz, MemPool& pool) {
  return pool.alloc(sz);
}
void operator delete(void* ptr, MemPool& pool) {
  // do nothing
}

static MemPool& pool() {
  static MemPool s_pool(1024 * 1024);
  return s_pool;
}

static vec3_t _vec3Attr(pugi::xml_node node, char const* name) {
  float v[3];
  assert(3==sscanf(node.attribute(name).value(), "%f%f%f", v, v+1, v+2));
  return vec3_t(v[0], v[1], v[2]);
}

static material_t _createMaterial(pugi::xml_node node) {
  return material_t {
    _vec3Attr(node, "color"),
    node.attribute("roughness").as_float(0),
    node.attribute("emit").as_bool(false)
  };
}

static Geometry* _createSphere(pugi::xml_node node) {
  assert(!strncmp( node.attribute("type").value(), "sphere", 7 ));
  Sphere *s = new(pool()) Sphere;
  s->center = _vec3Attr(node, "center");
  s->radius = node.attribute("radius").as_float(1.0f);
  return s;
}

static Geometry* _createPlane(pugi::xml_node node) {
  assert(!strncmp( node.attribute("type").value(), "plane", 6 ));
  Plane *p = new(pool()) Plane;
  p->center = _vec3Attr(node, "center");
  p->normal = _vec3Attr(node, "normal");
  return p;
}

static Geometry* _createDisk(pugi::xml_node node) {
  assert(!strncmp( node.attribute("type").value(), "disk", 5 ));
  Disk *d = new(pool()) Disk;
  d->center = _vec3Attr(node, "center");
  d->normal = _vec3Attr(node, "normal");
  d->radius = node.attribute("radius").as_float(1.0f);
  return d;
}

static Geometry* _createOrientedBox(pugi::xml_node node) {
  assert(!strncmp( node.attribute("type").value(), "orb", 4 ));
  OrientedBox *orb = new(pool()) OrientedBox;
  orb->center = _vec3Attr(node, "center");
  orb->axis[0] = normalize(_vec3Attr(node, "x-axis"));
  orb->axis[1] = normalize(_vec3Attr(node, "y-axis"));
  orb->axis[2] = normalize(_vec3Attr(node, "z-axis"));
  orb->extent  = _vec3Attr(node, "extent");
  return orb;
}

static std::unordered_map<std::string, Geometry* (*)(pugi::xml_node)> getRegistry() {
  std::unordered_map<std::string, Geometry* (*)(pugi::xml_node)> reg;
  reg["sphere"] = _createSphere;
  reg["plane"] = _createPlane;
  reg["disk"] = _createDisk;
  reg["orb"] = _createOrientedBox;

  return reg;
}

bool Scene::read(std::string const& filename) {
  pugi::xml_document xml;
  if (!xml.load_file(filename.c_str())) {
    return false;
  }
  for (Geometry* g:geometry_list) {
    g->~Geometry();
  }
  pool().reset();
  geometry_list.clear();
  material_list.clear();

  pugi::xml_node root = xml.child("scene");
  if (!root) {
    fprintf(stderr, "error: scene has no root <scene> node\n");
    return false;
  }
  static auto reg = getRegistry();
  for (pugi::xml_node mat=root.child("material"); mat; mat = mat.next_sibling("material")) {
    material_list[mat.attribute("name").value()] = _createMaterial(mat);
  }

  for (pugi::xml_node geo=root.child("geometry"); geo; geo = geo.next_sibling("geometry")) {
    std::string type = geo.attribute("type").value();
    if (reg.find(type) == reg.end()) {
      fprintf(stderr, "error: geometry of type %s not known\n", type.c_str());
      continue;
    }
    Geometry* g = reg[type](geo);
    if (material_list.find(geo.attribute("material").value()) == material_list.end()) {
      fprintf(stderr, "error: material named %s not found\n", geo.attribute("material").value());
      delete g;
      continue;
    }
    g->material = material_list[geo.attribute("material").value()];
    geometry_list.push_back(g);
  }

  pugi::xml_node cam = root.child("camera");
  if (!cam) {
    fprintf(stderr, "error: no camera found\n");
    return false;
  }
  camera.position = _vec3Attr(cam, "position");
  camera.direction = _vec3Attr(cam, "direction");
  camera.up = _vec3Attr(cam, "up");
  camera.fov = cam.attribute("fov").as_float(0.8f);
  camera.near = cam.attribute("near").as_float(0.1f);
  camera.far = cam.attribute("far").as_float(1000.0f);
  return true;
}


