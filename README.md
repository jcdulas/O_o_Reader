# O_o_Reader

A C++20 library, compiled to WebAssembly for the web, that:

- reads and writes its own **model format** (`.oos`, [docs/model-format.md](docs/model-format.md));
- stores models and results in its own **HDF5 archive** (`.ooh5`), together or separately: typed model columns,
  compressed buffers and model loading on demand ([docs/archive-format.md](docs/archive-format.md));
- writes results as an **Excel workbook** (.xlsx) for people: an index sheet and one sheet per table;
- exports custom query tables as **HDF5 datasets** (`.h5`);
- analyses a model (geometry, coordinate frames, mass, mesh quality, components, connections, validation checks) and
  runs over 160 **named queries** on a model and its results (inventory, loads, stresses, margins, section cuts, ...).

It is the reading layer of the O_o viewer and works with native model records, archives and their neutral names.
The project's rules on formats and third-party code are in
[LEGAL.md](LEGAL.md).

## Documentation

[JavaScript optimization](docs/optimization.md): design interfaces, derivatives and accepted model updates.

**[Read online](https://jcdulas.github.io/O_o_Reader/)** · [Architecture with diagrams](docs/architecture.md) · [Documentation index](index.md)

The Documentation workflow publishes the HTML site and provides a downloadable archive.
See [read, download and build instructions](docs/documentation.md).

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
results.writeArchive("/work/plate.ooh5");                         // to exchange and to view
results.writeXlsx("/work/plate.xlsx");                             // for people
const table = oor.openResults("/work/plate.ooh5").read(0);        // { shape, ids, values, ... }

// One file for the model and its results, without converting numeric model data to text.
results.attach(deck, false);
results.writeArchive("/work/complete.ooh5");
const complete = oor.openResults("/work/complete.ooh5");          // lists results without loading the model
const embedded = complete.model();                               // loads and attaches the model on demand
const sameModel = oor.readModel("/work/complete.ooh5");           // the model can also be read independently
deck.writeArchive("/work/model-only.ooh5");                       // a model without results
```

The whole interface: [docs/wasm-api.md](docs/wasm-api.md).

New `.ooh5` files use archive version **4**, with native model schema **1**. Version **3** result archives remain
readable. See [choosing a format](docs/getting-started.md#choose-a-file-format) and
[archive compatibility](docs/archive-format.md#versions) before updating an existing viewer.

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
