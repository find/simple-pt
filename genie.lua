solution("simple-pt")
language("C++")
location(".build")
targetdir(".build/bin")
debugdir(path.getabsolute("."))
debugargs('"test/scene.xml" -w 500 -h 200 -s 128 -o scene.ppm')
buildoptions({'-openmp'})
configurations({"Debug", "Release"})
platforms({"x32","x64"})
startproject("simple-pt")
configuration("Debug")
  flags({"Symbols"})
  targetsuffix("-d")
configuration("Release")
  flags({
    "EnableSSE",
    "FloatFast",
    "NoBufferSecurityCheck",
    "NoFramePointer",
    "NoRTTI",
    "OptimizeSpeed"
  })
configuration("x64")
  targetdir(".build/bin64")
configuration("vs*")
  defines({'_CRT_SECURE_NO_WARNINGS'})
configuration("")

project("pugixml")
kind("StaticLib")
files({"3rdparty/pugixml/**"})

project("docopt")
kind("StaticLib")
files({"3rdparty/docopt/**"})
removeflags({'NoRTTI'})

project("simple-pt")
kind("ConsoleApp")
files({"src/**.h", "src/**.cpp"})
links({'pugixml', 'docopt'})

