# Nirvana Port ibrary for Windows

This is a part of the Nirvana project.

This folder must be added to include paths
in the Nirvana Core project.

## Folders

### ./Port

Standard Nirvana porting headers.

### ./Source

The sources and internal headers to build Nirvana for Windows port library.

### ./msvccrt

The sources of the MSVC support library intended for building
Nirvana core without the MSVC UCRT library.

### ./msvcstl

The sources of the MSVC support library intended for building
Nirvana core without the MSVC STL library.

### ./Test

Sources for various tests.

### ./pe

Portable executable file format support.

## How to build port libraries

Before building the Nirvana core for Windows, have to build the following libraries.

### LibWindows

This library implements port Nirvana core to Windows API.

Use all files from ./Source folder to build it.

## How to build Nirvana core for windows

TBD

