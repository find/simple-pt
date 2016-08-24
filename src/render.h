#include "scene.h"
#include "math.h"

struct bitmap_t {
  vec3_t *pixels;
  int     width;
  int     height;
};

struct option_t {
  int     depth;
  int     samples;
};

bitmap_t createRenderTarget(int width, int height);
void     deleteRenderTarget(bitmap_t *bm);
bool     saveRenderTarget(char const* filename, bitmap_t const& bm);
void     renderLowQuality(bitmap_t *target, Scene const& scene, option_t const& opt);
void     render(bitmap_t *target, Scene const& scene, option_t const& opt);
