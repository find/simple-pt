#include "scene.h"
#include "render.h"
#include "../3rdparty/docopt/docopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

static const char USAGE[] = R"(
simple path tracer.

Usage:
  simple-pt (-? | --help)
  simple-pt --version
  simple-pt <scene> [--width=<w>] [--height=<h>] [--depth=<d>] [--samples=<n>] [--algo=fast|--algo=trace] [--output=<fn>]

Options:
  -?, --help           show this help
  --version            show version
  -w X, --width=X      image width  [default: 320]
  -h Y, --height=Y     image height [default: 200]
  -d depth, --depth=d  tracing depth (bounce times) [default: 6]
  -s n, --samples=n    number of samples per pixel [default: 512]
  -a f, --algo=f       rendering function, fast or trace [default: trace]
  -o f, --output=f     output file name [default: output.ppm]
)";

int main(int argc, char** argv)
{
  Scene scene;
  std::map<std::string, docopt::value> args;
  try {
    args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "simple tracer 0.0.1");
  } catch (std::exception const &e) {
    fprintf(stderr, "error: %s\n", e.what());
    return -1;
  }
  if (!scene.read(args["<scene>"].asString())) {
    fprintf(stderr, "failed to read scene %s\n", argv[1]);
    return 2;
  }
  bitmap_t bm = createRenderTarget(args["--width"].asLong(), args["--height"].asLong());
  option_t opt = {
    args["--depth"].asLong(),
    args["--samples"].asLong()
  };
  if (args["--algo"].asString() == "fast") {
    renderLowQuality(&bm, scene, opt);
  } else {
    auto start = std::chrono::high_resolution_clock::now();
    render(&bm, scene, opt);
    std::chrono::duration<double, std::milli> duration = std::chrono::high_resolution_clock::now() - start;
    fprintf(stdout, "rendering takes %.3fs\n", duration.count()/1000.0);
  }
  std::string ofn = args["--output"].asString();
  saveRenderTarget(ofn.c_str(), bm);
  deleteRenderTarget(&bm);
  system(ofn.c_str());
  return 0;
}

