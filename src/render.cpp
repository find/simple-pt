#include "render.h"
#include <stdio.h>
#include <random>

bitmap_t createRenderTarget(int width, int height) {
  bitmap_t rt = {
    nullptr,
    width,
    height
  };
  if (width>0 && height>0) {
    rt.pixels = new vec3_t[width*height];
    memset(rt.pixels, 0, sizeof(vec3_t)*width*height);
  }
  return rt;
}

void deleteRenderTarget(bitmap_t *bm) {
  if (!bm || !bm->pixels)
    return;
  delete[] bm->pixels;
  bm->width = 0;
  bm->height = 0;
}

static inline int tobyte(real_t r) {
  return int(clamp(r, 0, 1)*255 + 0.5);
}

bool saveRenderTarget(char const* filename, bitmap_t const& bm) {
  FILE* ppm = fopen(filename, "w");
  if (!ppm) {
    return false;
  }
  fprintf(ppm, "P3\n%d %d\n%d\n", bm.width, bm.height, 255);

  for(size_t i=0, len=bm.width*bm.height; i<len; ++i) {
    fprintf(ppm, "%d %d %d ", tobyte(bm.pixels[i].x),
                              tobyte(bm.pixels[i].y),
                              tobyte(bm.pixels[i].z));
    if ((i+1)%bm.width == 0) {
      fprintf(ppm, "\n");
    }
  }

  fclose(ppm);
  return true;
}

/// @brief: render the scene without tracing, for testing the scene graph
void renderLowQuality(bitmap_t *target, Scene const& scene, option_t const& opt) {
  // first calculate how the ray casts
  real_t const fovy = scene.camera.fov;
  real_t const fovx = fovy / target->height * target->width;
  real_t const focal_distance_pixel = target->height / std::tan(fovy / real_t(2));

  vec3_t const forward = normalize(scene.camera.direction);
  vec3_t const up = normalize(scene.camera.up);
  vec3_t const right = cross(up, forward);

  for(int ix=0; ix<target->width; ++ix) {
    for(int iy=0; iy<target->height; ++iy) {
      // screen space
      real_t const sx = (real_t(ix) - real_t(target->width) / real_t(2));
      real_t const sy = (real_t(iy) - real_t(target->height) / real_t(2));
      vec3_t const spos(sx, -sy, focal_distance_pixel);

      // world space
      real_t const wx = dot(spos, right);
      real_t const wy = dot(spos, up);
      real_t const wz = dot(spos, forward);
      vec3_t const dir = normalize(vec3_t(wx, wy, wz));

      ray_t ray = {
        scene.camera.position,
        dir
      };
      real_t minSquareDist = std::numeric_limits<real_t>::max();
      real_t squareDist;
      for (Geometry* g:scene.geometry_list) {
        intersection_t intersection;
        if (g->intersect(ray, &intersection) &&
            (squareDist=lengthSquare(intersection.intersection[0]-ray.origin))<minSquareDist) { // TODO: transparency
          minSquareDist = squareDist;
          target->pixels[iy*target->width + ix] = intersection.material->color * std::abs(dot(normalize(vec3_t(0,-1,1)), intersection.normal[0]));
          // target->pixels[iy*target->width + ix] = normalize(intersection.normal[0]*real_t(0.5) + vec3_t(0.5, 0.5, 0.5));
        }
      }
    }
  }
}

// taken from unreal engine
static vec3_t importanceSampleGGX(real_t randPhi, real_t randTheta, real_t roughness) {
  real_t const m = roughness * roughness;
  real_t const m2 = m*m;

  real_t const phi = PI * 2 * randPhi;
  real_t const cosTheta = std::sqrt((1 - randTheta) / (1 + (m2 - 1)*randTheta));
  real_t const sinTheta = std::sqrt(1 - cosTheta*cosTheta);
  
  return vec3_t(
    sinTheta * std::cos(phi),
    sinTheta * std::sin(phi),
    cosTheta
  );
}

static vec3_t importanceSampleBlinn(real_t randPhi, real_t reandTheta, real_t roughness) {
  real_t const m = roughness * roughness;
  real_t const n = real_t(2) / (m*m) - 2;
  
  real_t const phi =  PI * 2 * randPhi;
  real_t const cosTheta = std::pow(std::max(reandTheta, real_t(1e-5)), real_t(1) / real_t(n + 1));
  real_t const sinTheta = std::sqrt(real_t(1) - cosTheta*cosTheta);

  return vec3_t(
    sinTheta * std::cos(phi),
    sinTheta * std::sin(phi),
    cosTheta
  );
}

static vec3_t importanceSampleCos(real_t randPhi, real_t randTheta, real_t roughness) {
  real_t const phi = PI * 2 * randPhi;
  real_t const theta = PI * randTheta - PI / 2;
  return vec3_t(
    std::sin(theta) * std::cos(phi),
    std::cos(theta) * std::sin(phi),
    std::cos(theta)
  );
}

// taken from unreal engine
static vec3_t tangentToWorld(vec3_t vec, vec3_t normal) {
  vec3_t const up = std::abs(normal.z) < 0.999 ? vec3_t(0, 0, 1) : vec3_t(1, 0, 0);
  vec3_t const x = normalize(cross(up, normal));
  vec3_t const y = cross(normal, x);
  return x*vec.x + y*vec.y + normal*vec.z;
}

template <class RandomFunction>
static ray_t reflect(ray_t const& ray, vec3_t const& pos, vec3_t const& normal, material_t const* material, RandomFunction &f) {
  real_t const cosangle = dot(ray.direction, normal);
  vec3_t const refdir = ray.direction - normal*cosangle*real_t(2);
  if (material->roughness > real_t(1e-6)) {
    vec3_t const micro_normal = tangentToWorld(importanceSampleGGX(f(), f(), material->roughness), cosangle<0?normal:-normal);
    vec3_t const ref = ray.direction - real_t(2) * dot(ray.direction, micro_normal)*micro_normal;
    return ray_t{ pos, normalize(ref) };
  } else {
    return ray_t{ pos, refdir }; // È«·´Éä
  }
}

template <class RandomFunction>
static vec3_t radiance(ray_t const& ray, Scene const& scene, int depth, RandomFunction &f) {
  if (depth < 0) {
    return vec3_t(0, 0, 0);
  } else {
    real_t closest2 = std::numeric_limits<real_t>::max();
    vec3_t color(0, 0, 0);
    for (Geometry* g : scene.geometry_list) {
      intersection_t intr;
      real_t distance2;
      if (g->intersect(ray, &intr) && (distance2 = lengthSquare(intr.intersection[0] - ray.origin)) < closest2) {
        closest2 = distance2;
        ray_t ref = reflect(ray, intr.intersection[0], intr.normal[0], intr.material, f);
        ref.origin += ref.direction*real_t(1e-4);
        color = radiance(ref, scene, depth - 1, f);
        if (intr.material->emit) {
          color = color + intr.material->color;
        } else {
          color = color * intr.material->color;
        }
      }
    }
    return color;
  }
}

/// properly renders the scene
void render(bitmap_t *target, Scene const& scene, option_t const& opt) {
  // first calculate how the ray casts
  real_t const fovy = scene.camera.fov;
  real_t const fovx = fovy / target->height * target->width;
  real_t const focal_distance_pixel = target->height / std::tan(fovy / real_t(2));

  vec3_t const forward = normalize(scene.camera.direction);
  vec3_t const up = normalize(scene.camera.up);
  vec3_t const right = cross(up, forward);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<real_t> dist(real_t(0), real_t(1));
  auto random = [&gen, &dist]()->real_t {
    return dist(gen);
  };

#pragma omp parallel for schedule(dynamic,1)
  for (int ix = 0; ix<target->width; ++ix) {
    for (int iy = 0; iy<target->height; ++iy) {
      // super sampling
      for (int superx = 0; superx < 2; ++superx) for (int supery = 0; supery < 2; ++supery) {
        // screen space
        real_t const sx = real_t(ix + (superx - 0.5)/2.0 - real_t(target->width) / real_t(2));
        real_t const sy = real_t(iy + (supery - 0.5)/2.0 - real_t(target->height) / real_t(2));
        vec3_t const spos(sx, -sy, focal_distance_pixel);

        // world space
        real_t const wx = dot(spos, right);
        real_t const wy = dot(spos, up);
        real_t const wz = dot(spos, forward);
        vec3_t const dir = normalize(vec3_t(wx, wy, wz));

        ray_t ray = {
          scene.camera.position,
          dir
        };
        real_t minSquareDist = std::numeric_limits<real_t>::max();
        real_t squareDist;
        vec3_t pixelColor(0, 0, 0);
        for (Geometry* g : scene.geometry_list) {
          intersection_t intersection;
          if (g->intersect(ray, &intersection) &&
            (squareDist = lengthSquare(intersection.intersection[0] - ray.origin)) < minSquareDist) {
            minSquareDist = squareDist;
            pixelColor = vec3_t(0, 0, 0);

            for (int i = 0; i < opt.samples; ++i) {
              ray_t ref = reflect(ray, intersection.intersection[0], intersection.normal[0], intersection.material, random);
              pixelColor += radiance(ref, scene, opt.depth, random) * intersection.material->color * real_t(1.0 / opt.samples);
            }
          }
        }
        target->pixels[iy*target->width + ix] += pixelColor * real_t(0.25);
      }
    }
    fprintf(stdout, "rendering ... %.2f%%    \r", float(ix*100) / float(target->width));
  }
  fprintf(stdout, "done.                     \n");
}

