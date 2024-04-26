# LearnOpenGl Examples 

**Unofficial** cross platform examples for [learnopengl.com](https://learnopengl.com/)

[Live Demos](https://zeromake.github.io/learnopengl-examples)

- written in C.
- shader dialect GLSL v450
- runs on OSX, Linux, Windows and web (emscripten) from the same source
- uses [Sokol libraries](https://github.com/floooh/sokol) for cross platform support


## Building 

#### Requirements

* a **C development environment**:
    - OSX: Xcode + command line tools
    - Linux: gcc/clang
    - Windows: Visual Studio/Mingw
* [xmake](https://xmake.io/)

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
> export EMSDK_PATH=../emsdk
> $EMSDK_PATH/emsdk activate latest
> source $EMSDK_PATH/emsdk_env.sh
> xmake f -c -y -p wasm -a wasm64
> xmake b
> xmake lua webpage.lua
> # generate to docs dir
```

#### Todo

- [ ] [4-5-4-sharpen](src/4-5-framebuffers/4-sharpen.c)
- [ ] [4-5-5-blur](src/4-5-framebuffers/5-blur.c)
- [ ] [4-10-1-instancing](src/4-10-instancing/1-instancing.c)
