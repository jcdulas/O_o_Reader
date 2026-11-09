# O_o_Reader

A C++20 library, compiled to WebAssembly for the web, that:

- reads and writes its own **model format** (`.oos`, [docs/model-format.md](docs/model-format.md));
- writes results as an **Excel workbook** (.xlsx) for people: an index sheet and one sheet per table;
- stores results and custom outputs in an **HDF5 results archive** of its own layout, made for fast web viewing
  ([docs/archive-format.md](docs/archive-format.md));
- analyses a model (geometry, coordinate frames, mass, mesh quality, components, connections, validation checks) and
  runs over 160 **named queries** on a model and its results (inventory, loads, stresses, margins, section cuts, ...).

It is the reading layer of the O_o viewer. O_o_Converter imports and exports the Nastran bulk data input format;
the reader works with model records and their neutral names. The project's rules on formats and third-party code are in
[LEGAL.md](LEGAL.md).

## Documentation

[Documentation index](index.md) · [Download the private HTML site](https://github.com/jcdulas/O_o_Reader/actions/workflows/documentation.yml)

The Sphinx site is built by the Documentation workflow and downloadable as a private artifact.
See [build and download instructions](docs/documentation.md).

## Use it from JavaScript

```js
import { loadOorReader } from "oor-reader";

const oor = await loadOorReader();
oor.writeFile("/work/plate.oos", modelText);                      // a model file (docs/model-format.md)
const deck = oor.readModel("/work/plate.oos");
const nodes = oor.runQuery("nodesTable", { deck }).columns();     // ids, x, y, z, ...

const results = oor.createResults([                                // results computed by a program
  { kind: "displacements", subcase: 1, ids: [1, 2], values: new Float64Array(12) },
]);
results.writeArchive("/work/plate.oorh5");                         // to exchange and to view
results.writeXlsx("/work/plate.xlsx");                             // for people
const table = oor.openResults("/work/plate.oorh5").read(0);        // { shape, ids, values, ... }
```

The whole interface: [docs/wasm-api.md](docs/wasm-api.md).

## Layout of the repository

| Folder | What |
| --- | --- |
| `include/oor/<module>/`, `src/<module>/` | the library, in modules that each use only the ones before them: `base` → `io` → `deck` → `model` → `results` → `analysis` → `query` → `api` |
| `wasm/` | the WebAssembly module: the embind interface (`src/Bindings.cpp`), the JavaScript wrapper (`js/`), its Node.js test (`test/`) |
| `tests/cpp/<module>/` | the C++ tests (Catch2), run natively |
| `validation/models/` | model files written for the project, read by the tests |
| `docs/` | the archive format, the JavaScript interface, the architecture |
| `scripts/` | the build scripts |
| `tools/` | the layering check and the generator of the card catalogue (`oos_layouts/`) |
| `formats/` | the card layout catalogue of the model format (`oos-layouts.json`, [docs/model-format-cards.md](docs/model-format-cards.md)) |

Start reading at [docs/architecture.md](docs/architecture.md).

## Build

### Native (development and tests)

Needs a C++20 compiler (GCC 12+ or Clang 16+), CMake 3.25+, Ninja and libhdf5 (on Windows: MSYS2 UCRT64 with
`mingw-w64-ucrt-x86_64-hdf5`).

```bash
cmake --preset release            # build/release
cmake --build --preset release
ctest --test-dir build/release -j 8 --output-on-failure
```

or `scripts/check.sh` (build, tests, layering check).

### WebAssembly

The module is 64-bit WebAssembly (Memory64, up to 16 GB of memory). Needs the Emscripten SDK
(`emsdk install latest && emsdk activate latest`, then its `emsdk_env` script) and a static libhdf5 built for 64-bit
WebAssembly, made once by `scripts/build_hdf5_wasm.sh <work folder>`.

```bash
scripts/build_wasm.sh <work folder of build_hdf5_wasm.sh>   # build/wasm/dist: the npm package
node --test wasm/test/smoke.test.mjs                        # the end-to-end test, in Node.js 24+
```

## Licence

MIT (see [LICENSE](LICENSE)). The WebAssembly package includes HDF5 (BSD-style licence) and zlib (zlib licence).
