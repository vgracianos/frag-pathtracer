# frag-pathtracer
A (very hacky) path tracer written in a GLSL fragment shader

<div style="text-align:center;max-width:100%;height:auto;">
<div style="max-width:45%;display:inline-block;padding:1%">
   <img src="https://pbs.twimg.com/media/CUvazvXXIAAzGvS.jpg" style="max-width:100%"/>
</div>
<div style="max-width:45%;display:inline-block;padding:1%">
  <img src="https://pbs.twimg.com/media/CUvazu8WEAE7Szw.jpg" style="max-width:100%"/>
</div>
<div style="max-width:45%;display:inline-block;padding:1%">
  <img src="https://pbs.twimg.com/media/CUvazuPWUAEnHfc.jpg" style="max-width:100%"/>
</div>
<div style="max-width:45%;display:inline-block;padding:1%">
  <img src="https://pbs.twimg.com/media/CUvaz2GXAAAd7Vv.jpg" style="max-width:100%"/>
</div>
</div>

* Uses sphere-tracing (raymarching) to find intersections between rays and objects
* Supports Lambertian and Blinn-Phong BRDFs.
* Pure Specular and Transmissive materials using Schlick's approximation for the Fresnel term.
* Everything is computed by a (hacky) fragment shader ([Shadertoy](https://www.shadertoy.com) style, just for fun)

# Compiling

This project uses **GLFW** for window management and **GLEW** for handling OpenGL dependencies. GLFW is included in the source, but not its dependencies.

The source can be compiled using CMake and tested as follows:
```
cd frag-pathtracer/
mkdir build
cd build/
cmake ..
make
./pathtracer scenes/scene1.in
```

This software has been tested under **Ubuntu 14.04 LTS** using a **NVIDIA GTX 970** (Driver 367.44) graphics card.

# Source Files

Name          | Description
--------------|-------------------------------------------
doc/          | LaTeX source files
include/      | C/C++ headers
libraries/    | External libraries (GLFW)
scenes/       | Example scene files
shaders/      | GLSL source code for shaders
src/          | C/C++ source files
LEIAME.pdf    | Documentation in PDF format (PT-BR only)
