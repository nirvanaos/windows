# Visual C CRT support library

This library is intended for building the Nirvana core without the MSVC UCRT library.

**Currently it is under development and incomplete!**

## How to build

### Additional Include Directories
* [Nirvana Library](https://github.com/nirvanaos/library.git)/Include
* [Nirvana IDL Support Library](https://github.com/nirvanaos/orb.git)/Include
* $(VCToolsInstallDir)crt/src/vcruntime

### Code Generation

* Debug: Multi-threaded Debug (/MTd)
* Release: Multi-threaded (/MT)

### For CLang

`_ThrowInfo` is predefined type in MSVC compiler. CLang does not know it.
For compilation with CLang, add proprocessor definition `_ThrowInfo=ThrowInfo`.

## How to use

To use this library, add it to the Nirvana Core project libraries and add
MSVC UCRT libraries to /NODEFAULTLIB:

* Debug: libucrtd.lib
* Release: libucrt.lib

About the Microsoft Visual C default libraries see: https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features
