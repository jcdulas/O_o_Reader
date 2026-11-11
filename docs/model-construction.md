# Constructing models from JavaScript

An application can attach versioned text documents to a model with `deck.setDocument(name, text)`, list them with
`documentNames()`, read with `document(name)` and remove with `removeDocument(name)`. A copied deck owns its document
edits; `.ooh5` stores documents in each model part. The `.oos` writer reports documents it cannot represent.

`oor.createModel()` creates an empty C++ model directly in WebAssembly memory. Cards, analysis controls and load cases
can be supplied as JavaScript objects, without a virtual input file. These methods also work on imported models.
See [the API reference](api.rst) for the declarations.

## A complete model

```js
import { loadOorReader } from "oor-reader";

const oor = await loadOorReader();
const deck = oor.createModel();
deck.setSolution("LinearStatics");
deck.caseControl.global.set("TitleText", "Plate built in memory");
deck.caseControl.global.set("SinglePointConstrSelection", 7);
deck.caseControl.global.set("DisplacementOutput", "All", { options: ["PrintListing"] });
const loadCase = deck.caseControl.addSubcase(1, {
  commands: [{ name: "StaticLoadSelection", value: 10 }],
});

const nodes = deck.addCardBatch("Node", {
  id: new Int32Array([1, 2, 3, 4]),
  position1: new Float64Array([0, 1, 1, 0]),
  position2: new Float64Array([0, 0, 1, 1]),
  position3: 0, // a scalar column broadcasts to every row
});
const cards = deck.addCards([
  { type: "ShellQuad4", values: { id: 1, propId: 1, node1: 1, node2: 2, node3: 3, node4: 4 } },
  { type: "ShellProp", values: { id: 1, membraneMatId: 1, thickness: 0.01 } },
  { type: "IsoMat", fields: [1, 70000, null, 0.3] },
  { type: "SinglePointConstrList", values: { id: 7, components: 123456, nodes: [1, 4] } },
  { type: "PointForceLoad", fields: [10, 2, 0, 100, 0, 0, 1] },
]);

deck.applyDefaults();
const problems = deck.checkReferences();
if (problems.length) throw new Error(problems.map((problem) => problem.message).join("\n"));
console.log(loadCase.effective("SinglePointConstrSelection").value); // 7, inherited
deck.writeArchive("/work/plate.ooh5");
const bytes = oor.readFile("/work/plate.ooh5"); // bytes to download or store
deck.writeModel("/work/plate.oos");
deck.delete();
```

A load case selects the set ID of load cards, rather than owning copies of those cards. Multiple load cases can
select the same set. `LoadCombination` combines sets through its `terms` group (`termScaleFactor`, `loadSetId`).
Results can later be attached and saved with the model; see [the archive guide](archive-format.md).

## Cards, schemas and identity

`oor.cardTypes()` lists supported types. `oor.cardSchema(type)` returns a detached copy of the compiled layout:
field names and kinds, groups, sections, forms and defaults. Names are exact and case-sensitive. Unknown names are
errors; input objects cannot silently lose misspelled fields. `solutionCatalogue()` lists solution names and numbers;
`caseCommandCatalogue()` lists command shapes, variants, keywords and section/block roles for editor controls.

Each card definition supplies `type` and either `fields` or `values`:

| Member | Meaning |
| --- | --- |
| `fields` | Compact values in [model-record order](model-format-cards.md), without the card name. Covers every supported card layout. |
| `values` | Named scalar fields, repeated groups and sections. Omitted fields are blank and take the card reader's defaults. |
| `form` | Optional form name for named input; otherwise selected from its discriminating value. |
| `comment` | Optional comment text, without leading `#` characters. |

Scalars are finite numbers, strings or `null` for blanks. `{ real: 1 }` gives an integral number real semantics where
the distinction matters. Named `real` fields already do this automatically. Integer inputs must be safe JavaScript
integers; card readers also check their identifier ranges. Literal strings need no delimiter escaping.

Groups are arrays of objects; a one-field group also accepts scalar items. Counts and section markers come from
the catalogue. Fixed-count groups require their declared number of items. An explicit form must agree with its
discriminating values; slot-only forms are internal layouts. Sections carry their `key` and named values. For example:

```js
const rigid = deck.addCard({ type: "RigidInterpolation", values: {
  id: 50, referenceNode: 1, referenceComponents: 123,
  terms: [{ weight: 1, components: 123, nodes: [2, 3] }],
  parts: [{ key: "ThermalExpansion", alphaCoefficient: 1e-5, referenceTemperature: 20 }],
} });
```

`addCards(definitions)` returns `{ type, id, key }` per created entity. Some records create multiple entities:
`addCard(definition)` requires exactly one entity and returns its reference. Use `addCards` for expanding records. The existing text overload
`addCards(text)` remains available.

IDs may change, and load or constraint cards can share a set ID. Keys remain stable until removal and are never reused.
References are scoped to their model or part. Independent `copy()` models retain corresponding keys; unrelated
models do not share that key namespace. For example:

```js
const [force] = deck.addCards([{ type: "PointForceLoad", fields: [10, 2, 0, 100, 0, 0, 1] }]);
deck.setCardValues(force, { magnitude: 250 }); // one atomic scalar-field patch
deck.setCardComment(force, "Updated force");
const snapshot = deck.cardByKey(force);      // undefined after removal
deck.updateCard(force, { fields: [10, 3, 0, 250, 0, 0, 1] }); // complete replacement, same type/key
deck.removeCardRef(force);
```

`setCardValues` uses scalar names from `cardFieldNames`. For group or section changes use a complete `updateCard`.
Snapshots are detached values; fetch them again after edits. Existing category accessors such as `deck.nodes[id]`
still support property assignment, following the captured card key even after its ID changes.

## Controls and load cases

The root model owns executive and case sections. `setSolution()` accepts a number or neutral solution name.
`setExecutive(records)` replaces executive statements and file assignments together, after validation. Each record
has `name`, separate `fields` and an optional `comment`; `executiveRecords()` reads them back.

```js
const control = deck.caseControl;
const first = control.addSubcase(1);
first.setCommands([
  { name: "StaticLoadSelection", value: 10 },
  { name: "DisplacementOutput", value: "All", options: ["PrintListing"] },
]);
first.setSet(9, [1, "Through", 4]);
first.setParameter("PostprocessingFile", -1);
const step = first.addStep(1);
const inherited = step.effective("StaticLoadSelection");
const outputs = step.effectiveOutputs("DisplacementOutput");
first.renumber(3);
const snapshot = first.snapshot(); // key, kind, id, entries, children
first.remove("DisplacementOutput");
first.removeEntry(snapshot.entries[0].key);
first.removeSection();             // also removes its steps/substeps
```

Section keys survive unrelated edits and renumbering. Removed sections and children throw on access. `sections()`
returns the hierarchy, and `section(key)` retrieves a handle. `addSubcase(id, { kind })` also accepts combination,
symmetry and repeated-subcase kinds. `addAuxiliaryCase()` opens a separate scope for subsequent top-level subcases.
`addStep()` and `addSubstep()` create children. Numbers must be positive and unique for their kind in their scope.

Commands update by name and variant; outputs by name and option set; sets by ID; parameters by name. Global commands
are inherited unless overridden. Own output requests override the inherited requests of that name as a group.
`setCommands()` validates the entire batch before applying it.

`caseControl.replace(records)` supplies the complete structured case definition, including section and output-block
headers and specialized commands. Records have the `[case]` names and field order, without delimiters or bracketed
section lines. `caseControl.records()` reads that representation. Replacement is atomic and assigns fresh keys:
obtain all case handles, including `global`, again afterwards. Removing an imported section with Include markers
is refused to preserve its source structure.

## Includes and parts

```js
deck.addInclude("mesh/nodes.oos", [{ type: "Node", values: { id: 11, position1: 0 } }]);
const part = deck.addPart({ kind: "SuperelementPart", id: 5 });
part.addCards([{ type: "Node", values: { id: 11, position1: 99 } }]); // independent IDs
deck.writeModel("/work/assembly.oos", { splitIncludes: true });
part.delete();
```

`addInclude` appends a new relative `.oos` source and its cards. Its path must remain inside the model folder and
must not already exist in the source tree. Existing include trees are imported with `readModel()`. Includes flatten
in `toModelText()` and embed in `writeArchive()`.

`parts()` lists descriptors; `part({ kind, id, massId })` returns a handle or `null`. Part handles keep the root C++
model alive even when its JavaScript handle is deleted. Their card operations affect that part only. Create parts
on the root. `ActivePart` and `PassivePart` are unnumbered (ID zero). Removing or reordering parts and moving existing
cards between include sources are not exposed by this construction interface.

## Performance, validation and lifetime

- Use `addCardBatch(type, columns)` for large homogeneous meshes. Fixed-layout columns accept field names or
  one-based compact field numbers. Numeric typed arrays cross WASM once per column; scalars broadcast. Array lengths
  must agree. Supply `{ count }` when every column broadcasts. IDs and keys are returned as typed arrays.
- Use `addCards` for heterogeneous input or layouts with groups, sections or forms. Named input uses the cached
  catalogue. The generic `fields` representation covers every supported layout.
- Fixed layouts feed typed numeric slots directly to C++ readers. Complex layouts use the shared layout mapper.
  Neither path generates a whole model-file string. ID indexes and homogeneous column growth are reserved by batch.
- Edits validate before replacing values, then write directly to the affected columns. Work depends on the card,
  rather than the rows after it. Shared buffers clone on their first write. `deck.copy()` provides an independent copy with shared initial buffers,
  retaining keys; references can be used to address corresponding cards in that copy.
- Failed card or command batches leave live data unchanged. Temporary rows are tombstones; `compact()` reclaims
  them and rebuilds indexes without changing keys. Compact after sustained editing, not after every field.
- Forward references are allowed during construction. Call `applyDefaults()` after default cards are assembled,
  then `checkReferences()` when the model is ready. It returns fresh diagnostics separately from read diagnostics.
  Existing named validation queries provide analysis-specific checks.
- `oor.modelVersions(deck)` exposes `structure`, `content` and `control` counters for cache invalidation. Query large
  collections through `cardTable` or named queries instead of fetching individual snapshots in a loop.

Run substantial operations in a web worker. Returned typed arrays own their buffers and survive WASM memory growth.
Free native deck, result and query handles with `delete()`. Case handles are lightweight JavaScript objects; they
need no `delete()`, but require their deck handle to remain alive.

For a reproducible construction and sparse-edit workload, run:

```sh
node wasm/bench/model-construction.mjs
```

It constructs 200,000 nodes from typed columns, edits 1,000 scattered nodes, checks identifiers and coordinates, then
compacts the model. It reports one warm sample for each operation; compare runs on the same runtime and machine.

These interfaces leave data contract 75, archive format 4 and native model schema 1 unchanged.
