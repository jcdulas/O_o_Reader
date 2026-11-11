import assert from "node:assert/strict";
import path from "node:path";
import { test } from "./helpers/fast-test.mjs";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const dist = process.env.OOR_DIST ?? path.join(here, "..", "..", "build", "wasm", "dist");
const { loadOorReader } = await import(pathToFileURL(path.join(dist, "index.js")).href);
const oor = await loadOorReader();

test("application documents copy independently and survive a model archive", () => {
  const deck = oor.createModel();
  let copy, restored;
  try {
    deck.setSolution(101);
    deck.setDocument("optimizer.state", JSON.stringify({ iteration: 7, note: "café" }));
    copy = deck.copy(); copy.setDocument("optimizer.state", "new state");
    assert.equal(JSON.parse(deck.document("optimizer.state")).iteration, 7);
    assert.equal(copy.document("optimizer.state"), "new state");
    deck.writeArchive("/work/study-documents.ooh5");
    restored = oor.readModel("/work/study-documents.ooh5");
    assert.deepEqual(restored.documentNames(), ["optimizer.state"]);
    assert.equal(JSON.parse(restored.document("optimizer.state")).note, "café");
    assert.equal(restored.removeDocument("optimizer.state"), true);
    assert.equal(restored.document("optimizer.state"), null);
  } finally { restored?.delete(); copy?.delete(); deck.delete(); }
});

test("named aerodynamic values and labels survive a combined archive", () => {
  const deck = oor.createModel(), results = oor.createResults([]);
  let reopened, model;
  try {
    deck.setSolution(144); deck.setDocument("optimizer.state", "cycle 4");
    assert.ok(oor.namedResultCatalogue().some(entry => entry.kind === "aeroTrimVariables"));
    results.addNamed({ kind: "aeroTrimVariables", subcase: 2, analysisCode: 144,
      arrays: { variableType: { type: "int32", shape: [1], values: Int32Array.of(1) },
        statusCode: { type: "int32", shape: [1], values: Int32Array.of(3) },
        isFixed: { type: "int32", shape: [1], values: Int32Array.of(1) },
        value: { type: "float64", shape: [1], values: Float64Array.of(0.1) } },
      texts: { variable: ["AngleOfAttack"] } });
    assert.equal(results.metadata().namedCount, 1);
    results.attach(deck, false); results.writeArchive("/work/study-named.ooh5");
    reopened = oor.openResults("/work/study-named.ooh5"); model = reopened.model();
    assert.equal(reopened.named()[0].analysisCode, 144);
    const named = reopened.readNamed(0);
    assert.deepEqual(named.texts.variable, ["AngleOfAttack"]);
    assert.deepEqual(named.arrays.value.values, Float64Array.of(0.1));
    assert.equal(model.document("optimizer.state"), "cycle 4");
  } finally { model?.delete(); reopened?.delete(); results.delete(); deck.delete(); }
});

test("design capabilities and blank elastic constants are inspectable", () => {
  const deck = oor.createModel(); let table;
  try {
    deck.addCard({ type: "IsoMat", fields: [1, 1000, null, 0.3, 1] });
    deck.applyDefaults();
    table = oor.cardTable(deck, "IsoMat", ["id", "sourceBlankFields"]);
    assert.equal(table.column("sourceBlankFields").values[0], 2);
    const support = oor.designCapabilities(deck);
    assert.equal(support.modelDocuments, true);
    assert.equal(support.generalCardEditing, true);
    assert.ok(support.nativeCards.includes("IsoMat"));
  } finally { table?.delete(); deck.delete(); }
});
