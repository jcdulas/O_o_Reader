// Input adaptation is tested without a WASM runtime; model-editing.test.mjs covers native storage and validation.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { test } from "node:test";
import { installModelEditing } from "../js/model-editing.js";

class Deck {
  calls = 0;
  addCards(text) {
    return text;
  }
  _insertCard(card) {
    this.calls++;
    return card;
  }
  _insertCards(cards) {
    this.calls++;
    return cards;
  }
  _insertCardBatch(type, count, columns) {
    this.calls++;
    return { type, count, columns };
  }
}
const catalogue = readFileSync(
  new URL("../../formats/oos-layouts.json", import.meta.url),
  "utf8",
);
const editing = installModelEditing({ Deck, cardLayouts: () => catalogue });

test("named input preserves alternatives, numeric intent and literal strings", () => {
  const deck = new Deck();
  const card = deck.addCard({
    type: "ShellQuad4",
    values: {
      id: 7,
      node1: 1,
      node2: 2,
      node3: 3,
      node4: 4,
      materialAngle: 0,
    },
    comment: 'A; quoted "comment"',
  });
  assert.deepEqual(card.fields.slice(0, 9), [
    7,
    null,
    1,
    2,
    3,
    4,
    { real: 0 },
    null,
    null,
  ]);
  assert.equal(card.comment, 'A; quoted "comment"');
  assert.equal(deck.addCards("Node;1"), "Node;1");
  const node = deck.addCard({
    type: "Node",
    values: { id: 1, position1: -0, position2: 1e100 },
  });
  assert.ok(Object.is(node.fields[2].real, -0));
  assert.equal(node.fields[3].real, 1e100);
});

test("nested lists and sections carry their exact catalogue counts and order", () => {
  const card = new Deck().addCard({
    type: "RigidInterpolation",
    values: {
      id: 50,
      referenceNode: 1,
      referenceComponents: 123,
      terms: [{ weight: 1, components: 123, nodes: [2, 3] }],
      parts: [
        {
          key: "ThermalExpansion",
          alphaCoefficient: 1e-5,
          referenceTemperature: 20,
        },
      ],
    },
  });
  assert.deepEqual(card.fields, [
    50,
    1,
    123,
    1,
    { real: 1 },
    123,
    2,
    2,
    3,
    "ThermalExpansion",
    { real: 1e-5 },
    { real: 20 },
  ]);
});

test("a failed object batch does not call native insertion and schema copies are independent", () => {
  const deck = new Deck();
  assert.throws(
    () =>
      deck.addCards([
        { type: "Node", values: { id: 1 } },
        { type: "Node", values: { id: 2, typo: 3 } },
      ]),
    /typo/,
  );
  assert.equal(deck.calls, 0);
  assert.throws(
    () =>
      deck.addCard({ type: "ShellQuad4", values: { id: 1, matFrameId: 1.5 } }),
    /integer/,
  );
  assert.throws(
    () => deck.addCard({ type: "Node", values: { id: 1 }, fields: [1] }),
    /either/,
  );
  const schema = editing.cardSchema("Node");
  schema.fields[0].name = "changed";
  assert.equal(editing.cardSchema("Node").fields[0].name, "id");
  assert.ok(editing.cardTypes().includes("Node"));
});

test("column broadcasts, lengths and real-valued numeric input are checked before native work", () => {
  const deck = new Deck();
  const batch = deck.addCardBatch("Node", {
    id: new Int32Array([1, 2]),
    position1: 1e100,
  });
  assert.equal(batch.count, 2);
  assert.equal(batch.columns[1].broadcast, true);
  assert.deepEqual(batch.columns[1].data, { real: 1e100 });
  const calls = deck.calls;
  assert.throws(
    () => deck.addCardBatch("Node", { id: [1, 2], position1: [0] }),
    /length/,
  );
  assert.throws(() => deck.addCardBatch("Node", { typo: [1] }), /invalid/);
  assert.throws(
    () => deck.addCardBatch("LoadCombination", { id: [1] }),
    /fixed layout/,
  );
  assert.equal(deck.calls, calls);
});

test("large nested lists do not depend on the JavaScript function argument limit", () => {
  const nodes = Array.from({ length: 150000 }, (_, i) => i + 2);
  const card = new Deck().addCard({
    type: "RigidInterpolation",
    values: {
      id: 1,
      referenceNode: 1,
      referenceComponents: 123,
      terms: [{ weight: 1, components: 123, nodes }],
    },
  });
  assert.equal(card.fields[6], 150000);
  assert.equal(card.fields.at(-1), 150001);
});
