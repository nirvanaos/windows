# Nirvana Port ibrary for Windows

This is a part of the Nirvana project.

## Folders

### ./Port

Standard Nirvana porting headers. This folder must be added to include paths
in the Nirvana Core project.

### ./Source

The sources and internal headers to build Windows port library.

### ./MSVC

The sources of the MSVC support library intended for building
Nirvana core without the MSVC STL library.

#### Project Settings

##### C++/General/Additional Include Directories
* ../../../Library.VC/Nirvana/SRC/Include
* ../../../Library.VC/Nirvana/ORB/Include
* $(VCToolsInstallDir)crt/src/vcruntime

##### C++/Advanced/Forced Include File
* Nirvana\basic_string.h
* Nirvana\vector.h

##### For CLang

`_ThrowInfo` is predefined type in MSVC compiler. CLang does not know it.
For compilation with CLang, add proprocessor definition `_ThrowInfo=ThrowInfo`.

### ./Test

Sources for various tests.

## How to build Nirvana core for Windows host

https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features

