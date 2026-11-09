// End-to-end test of the WebAssembly package (64-bit WebAssembly), run with Node.js 24 or later:
//   node --test wasm/test/smoke.test.mjs  (OOR_DIST=<build>/dist, by default build/wasm/dist)
//
// It drives the package as a web application would: files put in the in-memory file system, a model read and queried,
// results made in JavaScript, written to an archive and read back, written to an Excel workbook, and query
// tables written to HDF5.

import assert from "node:assert/strict";
import path from "node:path";
import { test } from "node:test";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const dist = process.env.OOR_DIST ?? path.join(here, "..", "..", "build", "wasm", "dist");
const { loadOorReader } = await import(pathToFileURL(path.join(dist, "index.js")).href);
const oor = await loadOorReader();

// A plate of two four-node shells on six nodes, written for this test (a model file, docs/model-format.md).
const PLATE = `[executive]
SolutionSequence;101
[case]
SubcaseStart;1
DisplacementOutput;All
[model]
Node;1;;0.;0.;0.
Node;2;;1.;0.;0.
Node;3;;2.;0.;0.
Node;4;;0.;1.;0.
Node;5;;1.;1.;0.
Node;6;;2.;1.;0.
ShellQuad4;10;1;1;2;5;4
ShellQuad4;11;1;2;3;6;5
ShellProp;1;1;0.1
IsoMat;1;70000.;;0.33;2.7E-9
`;

// The displacements of the plate (static, subcase 1), as a solver would hand them over: (1 step, 6 rows, 6 components).
function plateDisplacements() {
  const values = new Float64Array(6 * 6);
  const t3 = [0, 1.0e-3, 3.0e-3, 0, 1.2e-3, 3.5e-3];
  t3.forEach((value, row) => (values[row * 6 + 2] = value));
  return { kind: "displacements", subcase: 1, ids: [1, 2, 3, 4, 5, 6], values, title: "PLATE" };
}

test("the module loads and gives its version", () => {
  assert.match(oor.version(), /^\d+\.\d+\.\d+$/);
});

test("a model is read, counted, queried and written", () => {
  oor.writeFile("/work/plate.oos", PLATE);
  const deck = oor.readModel("/work/plate.oos");
  assert.equal(deck.cardCounts().Node, 6);
  assert.equal(deck.cardCounts().ShellQuad4, 2); // card types by their neutral names
  assert.deepEqual(deck.diagnostics(), []);

  const nodes = oor.runQuery("nodesTable", { deck });
  assert.equal(nodes.rowCount(), 6);
  assert.equal(nodes.column("x").values[2], 2.0);

  const elements = oor.runQuery("elementsTable", { deck, args: { columns: ["elementId", "type", "nodes"] } });
  const connectivity = elements.column("nodes");
  assert.equal(connectivity.kind, "numberList");
  assert.deepEqual(Array.from(connectivity.offsets), [0, 4, 8]);
  assert.deepEqual(elements.column("type").values, ["ShellQuad4", "ShellQuad4"]);

  assert.deepEqual(deck.writeModel("/work/plate_copy.oos"), []);
  assert.match(oor.readFile("/work/plate_copy.oos", { encoding: "utf8" }), /^Node;1;/m);
  deck.delete();
});

test("a model path always holds model records", () => {
  oor.writeFile("/work/plate.oos", PLATE);
  const deck = oor.readModel("/work/plate.oos");
  assert.deepEqual(deck.diagnostics(), []);
  const text = deck.toModelText();
  assert.match(text, /^\[model\]$/m);
  assert.match(text, /^ShellQuad4;/m);
  assert.deepEqual(deck.writeModel("/work/again.oos"), []);
  assert.equal(oor.readFile("/work/again.oos", { encoding: "utf8" }), text);
  const model = oor.readModel("/work/again.oos");
  assert.deepEqual(model.diagnostics(), []);
  assert.deepEqual(model.cardCounts(), deck.cardCounts());
  assert.equal(model.toModelText(), text);
  assert.deepEqual(model.writeModel("/work/again.txt"), []);
  assert.equal(oor.readFile("/work/again.txt", { encoding: "utf8" }), text);
  assert.equal(oor.readModel("/work/again.txt").toModelText(), text);
  oor.writeFile("/work/bad.oos", "[model]\nNode;1;;0.;0.;0.;0;0;0;9\n");
  assert.equal(oor.readModel("/work/bad.oos").diagnostics()[0].severity, "error");
  model.delete();
  deck.delete();
});

test("results made in JavaScript are attached, archived, read back and written to a workbook", () => {
  oor.writeFile("/work/plate.oos", PLATE);
  const deck = oor.readModel("/work/plate.oos");
  const results = oor.createResults([plateDisplacements()], "plate");
  assert.equal(results.format(), "memory");

  const tables = results.tables();
  assert.equal(tables.length, 1);
  assert.equal(tables[0].kind, "displacements");
  assert.equal(tables[0].analysis, "static");
  const values = results.read(0, { components: ["t3"], ids: [6] });
  assert.deepEqual(values.shape, [1, 1, 1]);
  assert.ok(Math.abs(values.values[0] - 3.5e-3) < 1e-12);

  results.attach(deck, false);
  const maximum = oor.runQuery("maxDisplacement", { deck, results });
  assert.ok(maximum.rowCount() >= 1);

  const report = results.writeArchive("/work/plate.ooh5", { title: "PLATE" });
  assert.equal(report.finished, true);
  assert.deepEqual(report.skipped, []);
  const archive = oor.openResults("/work/plate.ooh5");
  assert.equal(archive.format(), "archive");
  assert.equal(archive.metadata().title, "PLATE");
  assert.equal(archive.read(0).values.length, 36);
  assert.equal(archive.hasModel(), true);
  const embedded = archive.model();
  assert.equal(embedded.toModelText(), deck.toModelText());
  assert.equal(embedded.nodes[6].xyz[0], 2);
  assert.equal(oor.readModel("/work/plate.ooh5").toModelText(), deck.toModelText());
  const unopenedModel = oor.openResults("/work/plate.ooh5");
  unopenedModel.writeArchive("/work/copied.ooh5");
  assert.equal(oor.readModel("/work/copied.ooh5").toModelText(), deck.toModelText());

  const workbook = archive.writeXlsx("/work/plate.xlsx", { title: "PLATE" });
  assert.equal(workbook.finished, true);
  assert.equal(workbook.written.length, 1);
  const bytes = oor.readFile("/work/plate.xlsx");
  assert.equal(bytes.length, workbook.bytes);
  assert.deepEqual(Array.from(bytes.slice(0, 2)), [0x50, 0x4b]); // "PK": a ZIP container

  // a single precision table stays single precision; a wrong table is refused with the reason
  const single = oor.createResults([{ ...plateDisplacements(), values: new Float32Array(36) }]);
  assert.equal(single.tables()[0].valueType, "float32");
  assert.throws(() => oor.createResults([{ kind: "displacements", ids: [1], values: [1, 2] }]), /bytes|values/);
  assert.throws(() => oor.createResults([{ kind: "nonsense", ids: [1], values: [] }]), /not a kind of result/);

  assert.throws(() => oor.openResults("/work/missing.ooh5"), /no such file/);
  assert.throws(() => oor.openResults("/work/plate.oos"), /not a results archive/);
  assert.throws(() => results.read(5), /no result table 5/);
});

test("a native archive can hold a model alone, and result-only archives remain supported", () => {
  oor.writeFile("/work/model-only.oos", PLATE);
  const deck = oor.readModel("/work/model-only.oos");
  assert.equal(deck.writeArchive("/work/model-only.ooh5").finished, true);
  const archive = oor.openResults("/work/model-only.ooh5");
  assert.equal(archive.hasModel(), true);
  assert.equal(archive.tables().length, 0);
  assert.equal(archive.model().toModelText(), deck.toModelText());
  const results = oor.createResults([plateDisplacements()]);
  results.writeArchive("/work/results-only.ooh5");
  const back = oor.openResults("/work/results-only.ooh5");
  assert.equal(back.hasModel(), false);
  assert.equal(back.model(), null);
  assert.throws(() => oor.readModel("/work/results-only.ooh5"), /contains no model/);
});

test("query tables are written to HDF5 and the catalogue is listed", () => {
  oor.writeFile("/work/plate.oos", PLATE);
  const deck = oor.readModel("/work/plate.oos");
  const nodes = oor.runQuery("nodesTable", { deck });
  oor.writeQueryTables("/work/outputs.h5", [{ name: "nodes", table: nodes }]);
  const bytes = oor.readFile("/work/outputs.h5");
  assert.deepEqual(Array.from(bytes.slice(1, 4)), [72, 68, 70]); // "HDF"
  assert.ok(nodes.toCsv().startsWith("nodeId,"));

  const catalogue = oor.queryCatalogue();
  assert.ok(catalogue.length > 100);
  assert.ok(catalogue.some((query) => query.name === "elementsTable"));
  assert.throws(() => oor.runQuery("nodesTable", { deck, args: { nonsense: 1 } }), /nonsense/);
});

test("cards are read by ID, edited in place, added and removed", () => {
  oor.writeFile("/work/plate.oos", PLATE);
  const deck = oor.readModel("/work/plate.oos");

  // read by ID
  const node = deck.nodes[3];
  assert.equal(node.type, "Node");
  assert.deepEqual(Array.from(node.xyz), [2, 0, 0]);
  assert.deepEqual(Array.from(node.xyzGlobal), [2, 0, 0]);
  assert.equal(deck.nodes[99], undefined);
  assert.deepEqual(deck.nodes.ids(), [1, 2, 3, 4, 5, 6]);
  const quad = deck.elements[11];
  assert.equal(quad.type, "ShellQuad4");
  assert.deepEqual(Array.from(quad.nodes), [2, 3, 6, 5]);
  assert.equal(deck.properties[1].thickness, 0.1);
  assert.equal(deck.cardById("IsoMat", 1).type, "IsoMat");
  assert.equal(deck.cardFields("nodes", 3)[0], "3");

  // edit in place: the card stays between Node 2 and Node 4
  deck.setCardField("nodes", 3, 3, "2.5");
  assert.deepEqual(Array.from(deck.nodes[3].xyz), [2.5, 0, 0]);
  const text = deck.toModelText();
  assert.ok(text.indexOf("Node;3;") < text.indexOf("Node;4;"));
  deck.replaceCard("ShellProp", 1, "ShellProp;1;1;0.25");
  assert.equal(deck.properties[1].thickness, 0.25);
  assert.throws(() => deck.setCardField("nodes", 3, 3, "abc"), /cannot be read/);
  assert.throws(() => deck.setCardField("nodes", 3, 1, "4"), /already used/);
  assert.deepEqual(Array.from(deck.nodes[3].xyz), [2.5, 0, 0]);

  // assign a field by its name
  deck.properties[1].thickness = 2;
  assert.equal(deck.properties[1].thickness, 2);
  const point = deck.nodes[3];
  point.position2 = 0.5;
  assert.deepEqual(Array.from(point.xyz), [2.5, 0.5, 0]); // the card object is read again
  assert.equal(deck.cardFieldNames("ShellProp", 1)[2].name, "thickness");
  assert.throws(() => {
    deck.properties[1].depth = 1;
  }, /its named fields are id, membraneMatId, thickness/);
  assert.throws(() => {
    point.xyz = [0, 0, 0];
  }, /one by one/);

  // add and remove
  const added = deck.addCards("Node;7;;3.;0.;0.\nNode;8;;3.;1.;0.\nShellQuad4;12;1;3;7;8;6");
  assert.deepEqual(added.map((card) => `${card.type} ${card.id}`), ["Node 7", "Node 8", "ShellQuad4 12"]);
  assert.equal(deck.elements[12].type, "ShellQuad4");
  assert.throws(() => deck.addCards("Node;9;;4.;0.;0.\nNode;1;;5.;0.;0."), /already used/);
  assert.equal(deck.nodes[9], undefined);
  assert.equal(deck.removeCard("elements", 12), true);
  assert.equal(deck.elements[12], undefined);
  deck.delete();
});
