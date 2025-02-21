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

- [ ] [4-1-4](src\4-1-depth-testing\4-linear-depth-buffer.c) 颜色偏黑了
- [ ] [4-2-1](src\4-2-stencil-testing\1-object-outlining.c) 直接就绿色正方体了，纹理没有
- [ ] [4-8-1](src\4-8-advanced-glsl\1-point-size.c) dx 下点比 gl 的小
- [ ] 5-3 dx 光源不对
- [x] 4-5 gl 与 dx 的 plane.model 坐标不相同
