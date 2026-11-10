// Run after building the WASM package: node wasm/bench/model-construction.mjs
// Reports one warm sample, not a universal latency guarantee. All generated data stays in memory.
import assert from "node:assert/strict";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
const here = path.dirname(fileURLToPath(import.meta.url));
const dist =
  process.env.OOR_DIST ?? path.join(here, "..", "..", "build", "wasm", "dist");
const { loadOorReader } = await import(
  pathToFileURL(path.join(dist, "index.js")).href
);
const oor = await loadOorReader();
const warm = oor.createModel();
warm.addCardBatch("Node", {
  id: new Int32Array([1, 2]),
  position1: new Float64Array([1, 2]),
});
warm.delete();
const count = 200000;
const ids = Int32Array.from({ length: count }, (_, i) => i + 1);
const x = Float64Array.from({ length: count }, (_, i) => i * 0.00123456789);
const deck = oor.createModel();
const start = performance.now();
const batch = deck.addCardBatch("Node", {
  id: ids,
  position1: x,
  position2: 0,
  position3: 0,
});
const constructed = performance.now();
const structure = oor.modelVersions(deck).structure;
for (let i = 0; i < 1000; ++i)
  deck.setCardValues(
    { type: "Node", key: batch.keys[i * 199] },
    { position1: -i },
  );
const edited = performance.now();
assert.equal(deck.cardCount(), count);
assert.equal(oor.modelVersions(deck).structure, structure);
assert.equal(
  deck.cardByKey({ type: "Node", key: batch.keys[999 * 199] }).values.xyz[0],
  -999,
);
const before = batch.keys[123];
deck.compact();
const compacted = performance.now();
assert.equal(batch.keys[123], before);
assert.equal(deck.cardByKey({ type: "Node", key: before }).id, 124);
console.log(
  JSON.stringify(
    {
      nodes: count,
      sparseEdits: 1000,
      constructionMs: constructed - start,
      editsMs: edited - constructed,
      compactMs: compacted - edited,
    },
    null,
    2,
  ),
);
deck.delete();
