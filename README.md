# LearnOpenGl Examples 

**Unofficial** cross platform examples for [learnopengl.com](https://learnopengl.com/)

[Live Demos](https://zeromake.github.io/learnopengl-examples)

- written in C.
- shader dialect GLSL v450
- runs on OSX, Linux, Windows and web (emscripten) from the same source
- uses [Sokol libraries](https://github.com/floooh/sokol) for cross platform support


## Building 

[Fips](http://floooh.github.io/fips/index.html) is used as build system to support multiple platforms.

#### Requirements

* a **C development environment**:
    - OSX: Xcode + command line tools
    - Linux: gcc/clang
    - Windows: Visual Studio/Mingw
* [xmake](https://xmake.io/)
* **Linux only:** libgl-dev libx11-dev libxi-dev libxcursor-dev

#### How to Build

```bash
> git clone https://github.com/zeromake/learnopengl-examples.git
> cd learnopengl-examples
> xmake f -c -y
> xmake b 1-3-1
> xmake r 1-3-1
```


#### Web Builds

To enable web builds you need to setup the [emscripten](https://emscripten.org/index.html) SDK

```bash
> xmake f -c -y wasm
> xmake b
> xmake lua webpage.lua
> # generate to docs dir
```

