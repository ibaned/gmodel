# Gmodel
> Create your own Gmsh models !

Gmodel is a C++11 library that implements a minimal CAD kernel
based on the `.geo` format used by the [Gmsh][0] mesh generation
code, and is designed to make it easier for users to quickly construct
CAD models for Gmsh.

## Installing / Getting started

You just need [CMake][0] and a C++11 compiler.

```shell
git clone git@github.com:ibaned/gmodel.git
cd gmodel
cmake . -DCMAKE_INSTALL_PREFIX=/your/choice
make install
```

This should install Gmodel under the given prefix in a way you can
access from your own CMake files using these CMake commands:

```cmake
find_package(Gmodel 1.2.0)
target_link_libraries(myprogram gmodel)
```

Several example executables are included and compiled along
with the library.

## Features

Gmodel provides at least the following:
* Polygon creation
* Extrusion of complex 2D faces into 3D objects
* Insertion of solids into other solids
* Built-in functions to make spheres, etc.
* Output model in Gmsh `.geo` format
* Output model in [PUMI][1] `.dmg` format

Gmodel does not support the more powerful CAD operations
involving objects whose boundaries overlap; we do not
currently have the resources to develop such features.

## Contributing

Please open a Github issue to ask a question, report a bug,
request features, etc.
If you'd like to contribute, please fork the repository and use a feature
branch. Pull requests are welcome.

## Licensing

This library is released under the FreeBSD license.

[0]: http://gmsh.info/
[1]: https://github.com/SCOREC/core
