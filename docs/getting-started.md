# Getting started

O_o_Reader reads model records, exposes typed tables and named queries, and manages result archives and workbooks.
Use [O_o_Converter](https://github.com/jcdulas/O_o_Converter/blob/main/index.md) when the input first needs conversion
to the model format.

## Build requirements

The [project overview](../README.md#build) gives the native and WebAssembly build commands.
Native development needs a C++20 compiler, CMake, Ninja and HDF5. The WebAssembly build also needs the Emscripten
SDK and HDF5 compiled for Memory64. Its package is written to `build/wasm/dist/`.

JavaScript use requires Node.js 24 or later, or a browser supporting Memory64. Serve the JavaScript wrapper and its
corresponding WebAssembly file together.

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
- [Results archive](archive-format.md): stored tables and custom outputs.
- [Table packages](table-package.md): transfer of query tables to a consumer.
- [Architecture](architecture.md) and [module guide](modules.md): implementation entry points.

Release native-backed objects with `delete()` when their data is no longer needed.
