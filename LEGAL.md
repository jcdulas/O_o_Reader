# Legal notes

The rules this project follows on file formats, names and third-party code. This document is not legal advice.

## File formats

- The library reads and writes the **O_o model format** (`.oos`), with neutral names for cards, fields, commands and
  parameter values.
- Models and results are stored in an **HDF5 layout of the project's own**
  ([docs/archive-format.md](docs/archive-format.md)); results are also written for people as **Excel workbooks**
  (Office Open XML, the open standard ISO/IEC 29500).
- The project neither reads nor writes the binary result formats or the result databases of other software vendors.

## Names

- The product, its namespace (`oor`), its libraries (`oor_*`), its package (`oor-reader`) and its file formats
  (`oor-results-archive`, `oor-query-tables`) carry
  no vendor name.
- The code, comments, messages and tests name no vendor and no vendor product.

## Rules for contributors

- The documentation of other software is not part of this repository: never add it, never quote it, never copy its
  examples into code, comments or tests.
- Test models and samples are written for the project, with values of its own.

## Third-party components

| Component | Use | Licence |
| --- | --- | --- |
| HDF5 (The HDF Group) | the HDF5 files (archive, query tables) | BSD-style (see COPYING in its sources) |
| zlib | the deflate filter of HDF5, the ZIP container of the workbooks | zlib licence |
| Emscripten | the WebAssembly build | MIT / University of Illinois NCSA |
| Catch2 | the C++ tests (not shipped) | Boost Software Licence 1.0 |

The WebAssembly package includes libhdf5 and zlib compiled to WebAssembly: ship their licence texts with it.
