# Using the WebAssembly package

The package `oor-reader` (built in `build/wasm/dist`) runs the whole library in a browser, a web worker or Node.js.
It is a WebAssembly module (`oor_reader_core.wasm`, made by Emscripten) and a small JavaScript wrapper
(`index.js` and `model-editing.js`, with its types in `index.d.ts`).

The module is **64-bit WebAssembly** (Memory64): it can use up to 16 GB of memory, where 32-bit WebAssembly stops at
4 GB, so large decks and result sets fit in memory with what is read from them. It needs a runtime that supports
Memory64: Node.js 24 or later, and a browser that ships the feature (check the browsers you target: a runtime without
it refuses to compile the module).

## Loading

```js
import { loadOorReader } from "oor-reader";

const oor = await loadOorReader();
console.log(oor.version()); // "0.2.0"
```

In a browser, serve `oor_reader_core.wasm` next to `oor_reader_core.js`, or tell Emscripten where it is:

```js
const oor = await loadOorReader({ locateFile: (file) => `/static/${file}` });
```

The module is single-threaded: run it in a web worker to keep the page responsive on large files.

## Files

The library reads and writes files in an in-memory file system. Put the input files in it, and take the output files
out of it:

```js
const text = await (await fetch("plate.oos")).text();
oor.writeFile("/work/plate.oos", text);          // a model file: also write the files it includes, at their paths
oor.writeFile("/work/run.ooh5", new Uint8Array(await file.arrayBuffer()));
// ... work ...
const bytes = oor.readFile("/work/run.ooh5");   // Uint8Array, to download or upload
oor.removeFile("/work/run.ooh5");               // free the memory of a file you no longer need
```

## Models

Use `oor.createModel()` to construct an empty model without a file. The [model construction guide](model-construction.md)
covers object input, typed batches, stable keys, load cases, controls, includes and parts. These APIs also edit imported models.

```js
const deck = oor.readModel("/work/plate.oos", { strict: false });  // a model file (model-format.md)
deck.cardCounts();        // { IsoMat: 1, Node: 6, ShellQuad4: 2, ShellProp: 1 }: card types by their neutral names
deck.diagnostics();       // [{ severity, code, message, location }]
deck.toModelText();
deck.writeModel("/work/out.oos", { splitIncludes: true });
deck.writeArchive("/work/model.ooh5"); // native typed model, without result tables
const nativeDeck = oor.readModel("/work/model.ooh5");
nativeDeck.delete();
deck.delete();            // free the C++ object when done
```

Model records, diagnostics and JavaScript values use the same exact neutral names
([naming.md](naming.md)): `ShellQuad4`, `propId`, `All` and `Yes`.

`readModel` detects a native `.ooh5` archive by its content and loads its typed model. Otherwise it reads model records,
regardless of the path's extension. An archive without a model raises an error. `writeModel` writes model records and
returns any content it could not carry, one description per item. `toModelText` gives one text with Include files flattened.

### Cards by ID

Every card type can be read by ID, with the names of its fields:

```js
const node = deck.nodes[11];        // undefined when there is no grid point 11
node.xyz;                           // Float64Array [x1, x2, x3] in the CP system of the grid point
node.xyzGlobal;                    // its position in the basic system
node.positionFrameId;               // the other fields, by name
deck.elements[10].nodes;            // the grid points of element 10
deck.properties[1].thickness;       // the thickness of ShellProp 1
deck.materials[1].e;
deck.coords[5];
deck.cardById("SinglePointConstrList", 7);           // any card type ("source"); a set card gives the first card of its ID
deck.nodes.ids();                   // [1, 2, 3, ...]
```

A card is `{ type, id, key, values, fields, text, comment }`; each named value is also a property of the card (`node.xyz` is
`node.values.xyz`). `fields` are the fields as written, field 1 first: the numbering the editing functions use.

### Editing cards

```js
deck.setCardField("nodes", 11, 3, "2.5");          // compact field 3 of a Node (position1) set to 2.5
deck.setCardFields("ShellProp", 1, ["1", "1", "0.25"]);
deck.properties[1].thickness = 0.25;                // a field by its name: the card is updated in the deck
deck.nodes[11].position1 = 2.5;
deck.setCardValue("ShellProp", 1, "bendingMatId", "3"); // the same, as a function
deck.cardFieldNames("ShellProp", 1);              // [{ field: 1, name: "id" }, { field: 2, name: "membraneMatId" }, ...]
deck.replaceCard("ShellProp", 1, "ShellProp;1;1;0.25");  // one model record of the same card type
deck.addCards("Node;7;;3.;0.;0.\nShellQuad4;12;1;3;7;8;6");   // [{ type: "Node", id: 7 }, { type: "ShellQuad4", id: 12 }]
deck.removeCard("elements", 12);
```

An edited card keeps its place in the deck, its comment and its link to attached results. Its new fields are checked as
the model reader checks them: a field the card cannot hold, or an ID another card has, throws an `Error` that says why,
and nothing changes. `addCards` adds all its cards or none.

A property can be assigned when it is the exact neutral name listed by `cardFieldNames`, such as `thickness` or
`position1`. The card is read again, so its properties show the new state. A value with several fields, such as
`xyz`, is set field by field (`position1`, `position2`, `position3`). Unknown field names throw an `Error` that lists
the card's fields. `fields` and numerical edits use the compact order of the model record, including separate fields
for alternatives such as `materialAngle` and `matFrameId`.

## Results

```js
const results = oor.openResults("/work/run.ooh5"); // an HDF5 results archive, recognised by its content
results.format();          // "archive"
const tables = results.tables();
// [{ index, kind: "displacements", elementType: "", subcase: 1, analysis: "static", stepKind, steps, rowCount,
//    components: ["t1", "t2", "t3", "r1", "r2", "r3"], valueType: "float64", ... }]

const data = results.read(tables[0].index, { stepIndices: [0], components: ["t3"], ids: [5, 6] });
// { shape: [1, 2, 1], ids: Int32Array, values: Float64Array, steps, layers, nodeIds, elementIds, texts }
// values[(step * rows + row) * components + component]; complex values: a last dimension of 2 (real, imaginary)

results.writeArchive("/work/run.ooh5", { compressionLevel: 4, title: "PLATE" });
results.writeXlsx("/work/run.xlsx", { title: "PLATE" });  // an Excel workbook, for people
```

`openResults` loads the result indexes, leaving model columns and result values unread. `model()` caches its deck;
subsequent calls share that deck and its edits. `attach(deck, false)` selects an existing deck instead. `writeArchive`
includes the attached deck, and preserves an embedded model even when `model()` has not been called. To write the model
alone, use `deck.writeArchive(path, options)`; it takes the same compression and title options. Archives containing only
results remain supported. New archives use version 4; version 3 result archives remain readable.

### A model and results in one archive

```js
results.attach(deck, false);
results.writeArchive("/work/complete.ooh5", { compressionLevel: 4 });

const archive = oor.openResults("/work/complete.ooh5");
if (archive.hasModel()) {
  const embedded = archive.model(); // load once; later calls share the same C++ deck
  try {
    const maximum = oor.runQuery("maxDisplacement", { deck: embedded, results: archive });
    try {
      console.log(maximum.toCsv());
    } finally {
      maximum.delete();
    }
    embedded.writeModel("/work/export.oos");
    embedded.writeArchive("/work/model-only.ooh5");
  } finally {
    embedded.delete();
  }
}
archive.delete();
```

| Operation | Behavior |
| --- | --- |
| `oor.openResults(path)` | Opens a model-only, result-only or combined archive and reads result indexes. |
| `results.hasModel()` | Reports an attached or embedded model without reading its columns. |
| `results.model()` | Returns the attached deck, or loads, caches and attaches the embedded deck; `null` when absent. |
| `oor.readModel(path, { strict })` | Reads `.oos` text or the native model independently; an archive without a model raises an error. |
| `results.attach(deck, strict)` | Selects a deck for model/result queries and subsequent archive writes. |
| `results.writeArchive(path, options)` | Includes the attached model, or preserves the embedded one if it has not yet been loaded. |
| `deck.writeArchive(path, options)` | Writes a model-only archive. |

Edits to the deck returned by `model()` are included by a later `results.writeArchive`. Calling `oor.readModel` instead
creates an independent deck: attach that deck explicitly if its edits should replace the model saved with the results.
`hasModel()` checks presence; a malformed or unsupported model schema raises an error when its model is loaded.

Both archive writers accept `ArchiveWriteOptions`: `compressionLevel` (0 to 9, default 4), `shuffle` (default true),
`strict` (default false), `title`, `subtitle` and `label`. `strict` raises an error instead of reporting skipped result
tables. Unsupported model control data always raises an error rather than being omitted. Cards from Include files
and their source metadata are embedded; files merely referenced by other card or control values remain references.

The [native model specification](archive-format.md#native-model) describes typed columns, sparse fields, constants,
source precision and pending defaults. The [getting-started example](getting-started.md#save-and-reopen-a-model-with-results)
is a complete runnable program. The archive, native model and table-package versions are separate contracts.

### Results made by a program

A solver, or a computation in the page, hands its results over as plain arrays; each table is checked against the
result catalogue (the components of each kind and element type):

```js
const results = oor.createResults([
  {
    kind: "displacements", subcase: 1, stepKind: "time", steps: [0.0, 0.1],
    ids: [1, 2, 3], values: new Float32Array(2 * 3 * 6),       // (steps, rows, components), single precision kept
  },
  {
    kind: "stress", elementType: "ShellQuad4", subcase: 1,
    ids: [10, 10], layers: [1, 2], values: new Float64Array(2 * 8),
  },
], "my run");
```

The members of a table are listed by `MemoryTable` in `index.d.ts`. Writing folders that do not exist yet are made.

### The Excel workbook

`writeXlsx` writes an index sheet (every table, with a link to its sheet) and one sheet per table: what the table is,
then a line per step and row with its labels (step, ID, grid point, layer, ...) and a column per component (two for a
complex value: real and imaginary parts), under a frozen header line with filters. A table longer than a sheet
(1,048,576 rows) goes on in the next sheets. The workbook is an output for people: the archive is the format to
exchange results.

Both writers return a report: `{ finished, bytes, written: [...], skipped: [{ kind, container, reason, message }] }`.
A table a format cannot hold is listed in `skipped`, never dropped silently.

## Queries

The named queries of the library (over 160: inventory of the model, geometry, mass, quality, loads, results, margins,
section cuts, ...) run on a deck, on results, or on both:

```js
const nodes = oor.runQuery("nodesTable", { deck });
nodes.column("x").values;                   // Float64Array of the basic x coordinates

const elements = oor.runQuery("elementsTable", { deck, args: { columns: ["elementId", "type", "propId", "nodes"] } });
const nodesOf = elements.column("nodes");   // { kind: "numberList", values, offsets }

results.attach(deck, false);                // link the results to the cards of the deck
const worst = oor.runQuery("maxDisplacement", { deck, results });

oor.queryCatalogue();                       // every query, its description and parameters
worst.toCsv();
```

Arguments are plain JavaScript values: numbers, texts, arrays of IDs, objects for named sets
(`{ wing: [1, 2, 3] }`), `{ start, stop, step }` for step positions. See `argumentsFromJson` in
[include/oor/api/Queries.hpp](../include/oor/api/Queries.hpp) for every kind.

### Custom outputs in HDF5

```js
oor.writeQueryTables("/work/outputs.h5", [
  { name: "nodes", table: nodes },
  { name: "worst_displacement", table: worst },
]);
```

The layout of that file is in [archive-format.md](archive-format.md#query-tables-custom-outputs).

## Card tables

The named queries give views made for people. A program that computes on the model (the O_o_Solver module) reads the
cards themselves: every field of a card type, with the computed columns it asks for, sorted by ID.

```js
oor.cardTypesOf(deck, "elements");                       // ["ShellQuad4", "ShellTria3"]
oor.cardTableColumns("ShellQuad4");                       // { stored: ["id", "propId", "nodes", ...], computed: [...] }
const quads = oor.cardTable(deck, "ShellQuad4", ["id", "propId", "nodes", "nodesRows", "propIdRow", "propType"]);
const nodes = oor.cardTable(deck, "nodes", ["id", "positionFrameId", "displacementFrameId", "permanentConstraints",
                                             "superelementId", "xyzGlobal"]);
oor.displacementRotations(deck);                         // Float64Array, 9 values per grid point (row-major)
oor.modelVersions(deck);                                 // { structure, content, control }
const bytes = quads.toPackage();                         // Uint8Array: the table with its exact Arrow layout
```

`nodesRows` indexes the rows of `cardTable(deck, "nodes")`, and `propIdRow` the rows of the property's card table.
A table package hands a table to another WebAssembly module: [table-package.md](table-package.md).

## Optimization designs

`designVariables(deck)` inspects bounds and dependencies, `optimizationTables(deck)` exports owned design table
packages, and `applyDesignPoint(deck, ids, values)` atomically writes an accepted point. The
[optimization guide](optimization.md) describes the Solver connection, validation and shape update rules.

## Errors

Every error of the library is thrown as a JavaScript `Error` whose message says what went wrong:

```js
try {
  oor.openResults("/work/missing.ooh5");
} catch (error) {
  console.error(error.message); // "cannot open /work/missing.ooh5: no such file"
}
```
