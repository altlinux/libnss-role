# libnss-role

## Contents

* [Introduction](#introduction)
* [Dependencies](#dependencies)
* [Build](#build)

* * *

## Introduction

**libnss-role** is an NSS libc module that implements adding groups to
groups for Linux / Unix systems and also provides functionality similar to
Windows' "Restricted Groups".

## Dependencies

* **CMake 3.10+** for building
* **cmocka** for unit testing
* **Ragel 6.10+** for regeneration of `/etc/role` parser

## Build

The build process is as simple as:

```
mkdir build
cd build
cmake ..
make
make test
```

