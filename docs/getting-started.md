# Getting started

O_o_Reader reads `.oos` model records and native `.ooh5` model/results archives, exposes typed tables and named
queries, and exports result workbooks.

## Build requirements

The [project overview](../README.md#build) gives the native and WebAssembly build commands.
Native development needs a C++20 compiler, CMake, Ninja and HDF5. The WebAssembly build also needs the Emscripten
SDK and HDF5 compiled for Memory64. Its package is written to `build/wasm/dist/`.

JavaScript use requires Node.js 24 or later, or a browser supporting Memory64. Serve the JavaScript wrapper and its
corresponding WebAssembly file together.

## Choose a file format

| Format | Contents | Main entry points |
| --- | --- | --- |
| `.oos` | Editable UTF-8 model records, with optional Include files | `oor.readModel`, `deck.writeModel` |
| `.ooh5` | Native typed model columns, results, or both in one HDF5 archive | `deck.writeArchive`, `results.writeArchive`, `oor.openResults`, `oor.readModel` |
| `.xlsx` | Result tables in a workbook for people | `results.writeXlsx` (output only) |
| `.h5` query export | Named query tables in the separate `oor-query-tables` layout | `oor.writeQueryTables` |

Use `.oos` for reviewing and editing records as text. Use `.ooh5` to save a model and its results together and load
numeric fields directly. An arbitrary HDF5 file is not a native model archive: renaming its extension does not convert
its contents. Table packages produced by `toPackage()` are a separate transport between Reader and Solver.

## Read and query a model

Save this example as `read-model.mjs` in the repository root after building the WebAssembly package:

~~~javascript
import { loadOorReader } from "./build/wasm/dist/index.js";

const oor = await loadOorReader();
const text = "[model]\nNode;1;;0.;0.;0.\n";
oor.writeFile("/work/point.oos", text);
const deck = oor.readModel("/work/point.oos");
const nodes = oor.runQuery("nodesTable", { deck });
try {
  if (deck.diagnostics().some((item) => item.severity === "error")) {
    throw new Error("The model contains an error.");
  }
  console.log("Nodes:", nodes.rowCount());
  console.log(deck.toModelText());
} finally {
  nodes.delete();
  deck.delete();
}
~~~

Run it with `node read-model.mjs`. It prints one node and the canonical model text.
The paths under `/work` belong to the module's in-memory file system; they are not host file paths.

To save a native binary model, call `deck.writeArchive("/work/point.ooh5")` and retrieve its bytes with
`oor.readFile`. `oor.readModel("/work/point.ooh5")` reads that model directly from typed HDF5 columns. An archive can
also include results: attach the deck to a result set and call `results.writeArchive`. See the
[archive specification](archive-format.md) for the layout and access patterns.

## Save and reopen a model with results

Save this example as `archive-model.mjs` in the repository root after building the WebAssembly package:

~~~javascript
import { loadOorReader } from "./build/wasm/dist/index.js";

const oor = await loadOorReader();
oor.writeFile("/work/point.oos", "[model]\nNode;1;;0.;0.;0.\n");
const deck = oor.readModel("/work/point.oos");
const results = oor.createResults([
  { kind: "displacements", subcase: 1, ids: [1], values: new Float64Array([0.1, 0, 0, 0, 0, 0]) },
]);

try {
  results.attach(deck, false);
  results.writeArchive("/work/point.ooh5");
  const archive = oor.openResults("/work/point.ooh5"); // result index only; model values stay unread
  const restored = archive.model();                  // loads the model and attaches it for queries
  try {
    console.log("Nodes:", restored.cardCounts().Node);
    console.log("Displacement:", archive.read(0).values[0]);
    restored.writeModel("/work/export.oos");
    console.log(oor.readFile("/work/export.oos", { encoding: "utf8" }));
  } finally {
    restored.delete();
    archive.delete();
  }
} finally {
  results.delete();
  deck.delete();
}
~~~

Run it with `node archive-model.mjs`. `oor.readFile("/work/point.ooh5")` returns the archive bytes for downloading or
writing to host storage. `oor.readModel("/work/point.ooh5")` can load the model independently of its results.

For a model alone, call `deck.writeArchive`. For results alone, write a result set with no attached model.
`archive.hasModel()` checks whether a model is available; `archive.model()` returns `null` when absent, whereas
`oor.readModel` raises an error for an archive without a model. See [archive versions](archive-format.md#versions):
new writes use version 4, and version 3 result archives remain readable.

## Model conventions

- A record occupies one physical line, with semicolons between values.
- Card names, field names and vocabulary words use their exact neutral spelling.
- Quote values containing semicolons or quotes; double an embedded quote.
- Use `Include` records for model includes. Put every required file in the module file system before reading.
- Inspect diagnostics before using a model. The C++ facade also offers strict read settings.

See the [format specification](model-format.md) and the [card catalogue](model-format-cards.md) for the ordered values,
defaults, alternatives and repeated groups of each card.

## Continue

- [JavaScript guide](wasm-api.md): file operations, queries, card editing and result objects.
- [API declarations](api.rst): current TypeScript types and C++ facade headers.
- [Native model/results archive](archive-format.md): model columns, result tables, compatibility and custom outputs.
- [Table packages](table-package.md): transfer of query tables to a consumer.
- [Architecture](architecture.md) and [module guide](modules.md): implementation entry points.

Release native-backed objects with `delete()` when their data is no longer needed.
