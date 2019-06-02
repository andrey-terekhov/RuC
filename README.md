# RuC
[![Build Status](https://travis-ci.org/andrey-terekhov/RuC.svg?branch=master)](https://travis-ci.org/andrey-terekhov/RuC)

Yet another compiler from Russian version of C into virtual machine.

## Building
After you download the repository, you might need to initialize submodules (fltk):
```
$ git submodule init
$ git submodule update
```

In order to build the RuC compiler you need CMake (at least 3.0.0). CMake is a tool that can generate Makefiles as well as IDE projects (MSVS solutions, CLion and Xcode projects), regardless of the platform or the compiler.

CMake can be installed via [Homebrew](http://brew.sh) on macOS:

```
$ brew install cmake
```

And via apt on Linux:

```
$ sudo apt-get -y install cmake
```

Alternatively, it can be downloaded from the [official site](https://cmake.org/download/).

When you've installed CMake, run the following commands from the root of this repository:

```
$ mkdir Build
$ cd Build
$ cmake ..
```

This will generate a Makefike, so you can use the standard routine:

```
$ make
$ make test
```

If you want to generate an IDE project (and not a Makefile), than instead of `cmake ..` run the following:

```
$ cmake .. -G <generator>
```

where `<generator>` is the name of the generator to use. To see the list of the available generators, run

```
$ cmake --help
```

For example, to generate an Xcode project on macOS, use this command:

```
$ cmake .. -G Xcode
```

The Xcode project will appear in the current directory, you can open it.

```
$ open RuC.xcodeproj
```
