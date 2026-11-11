import assert from "node:assert/strict";
import path from "node:path";
import { test } from "./helpers/fast-test.mjs";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const dist =
  process.env.OOR_DIST ?? path.join(here, "..", "..", "build", "wasm", "dist");
const { loadOorReader } = await import(
  pathToFileURL(path.join(dist, "index.js")).href
);
const oor = await loadOorReader();

test("a complete model is built from objects, solved-input tables and archives keep its cases", () => {
  const deck = oor.createModel();
  deck.setSolution("LinearStatics");
  deck.caseControl.global.set("TitleText", "Built; in memory");
  deck.caseControl.global.set("SinglePointConstrSelection", 7);
  deck.caseControl.global.set("DisplacementOutput", "All", {
    options: ["PrintListing"],
  });
  const first = deck.caseControl.addSubcase(1, {
    commands: [{ name: "StaticLoadSelection", value: 10 }],
  });
  deck.caseControl.addSubcase(2, {
    commands: [{ name: "StaticLoadSelection", value: 20 }],
  });
  const vertices = deck.addCardBatch("Node", {
    id: new Int32Array([1, 2, 3, 4]),
    position1: new Float64Array([0, 1, 1, 0]),
    position2: new Float64Array([0, 0, 1, 1]),
    position3: 0,
  });
  assert.deepEqual(Array.from(vertices.ids), [1, 2, 3, 4]);
  deck.addCards([
    {
      type: "ShellQuad4",
      values: {
        id: 1,
        propId: 1,
        node1: 1,
        node2: 2,
        node3: 3,
        node4: 4,
        materialAngle: 0,
      },
    },
    { type: "ShellProp", values: { id: 1, membraneMatId: 1, thickness: 0.01 } },
    { type: "IsoMat", fields: [1, 70000, null, 0.3] },
    {
      type: "SinglePointConstrList",
      values: { id: 7, components: 123456, nodes: [1, 4] },
    },
    { type: "PointForceLoad", fields: [10, 2, null, 100, 0, 0, 1] },
    {
      type: "LoadCombination",
      values: {
        id: 20,
        scaleFactor: 1,
        terms: [{ termScaleFactor: 2, loadSetId: 10 }],
      },
    },
  ]);
  assert.deepEqual(deck.checkReferences(), []);
  assert.equal(first.effective("SinglePointConstrSelection").value, 7);
  const table = oor.runQuery("subcasesTable", { deck });
  assert.equal(table.rowCount(), 2);
  table.delete();
  const before = deck.toModelText();
  assert.equal(deck.writeArchive("/work/built.ooh5").finished, true);
  const reopened = oor.readModel("/work/built.ooh5");
  assert.equal(reopened.toModelText(), before);
  reopened.delete();
  deck.writeModel("/work/built.oos");
  const textDeck = oor.readModel("/work/built.oos", { strict: true });
  assert.equal(textDeck.toModelText(), before);
  textDeck.delete();
  deck.delete();
});

test("batch validation and named fields reject mistakes without partial changes", () => {
  const deck = oor.createModel();
  const empty = deck.toModelText();
  assert.throws(
    () => deck.addCardBatch("Node", { id: new Int32Array([1, 1]) }),
    /already used/,
  );
  assert.equal(deck.toModelText(), empty);
  deck.addCard({ type: "Node", values: { id: 1 } });
  const before = deck.toModelText();
  assert.throws(
    () =>
      deck.addCardBatch("Node", {
        id: new Int32Array([2, 1]),
        position1: new Float64Array([1, 2]),
      }),
    /already used/,
  );
  assert.equal(deck.toModelText(), before);
  assert.throws(
    () => deck.addCardBatch("Node", { id: [2, 3], position1: [0] }),
    /length/,
  );
  assert.throws(
    () =>
      deck.addCards([
        { type: "Node", values: { id: 2 } },
        { type: "Node", values: { id: 3, typo: 1 } },
      ]),
    /typo/,
  );
  assert.equal(deck.toModelText(), before);
  assert.throws(
    () => deck.addCard({ type: "Node", fields: [2, 0, Infinity] }),
    /finite/,
  );
  assert.throws(
    () =>
      deck.addCard({
        type: "ShellQuad4",
        values: {
          id: 5,
          node1: 1,
          node2: 2,
          node3: 3,
          node4: 4,
          materialAngle: 1,
          matFrameId: 2,
        },
      }),
    /alternatives/,
  );
  assert.equal(deck.toModelText(), before);
  const emptyBatch = deck.addCardBatch("Node", { id: new Int32Array() });
  assert.equal(emptyBatch.keys.length, 0);
  deck.delete();
});

test("stable keys distinguish shared load-set IDs and survive sparse edits and compaction", () => {
  const deck = oor.createModel();
  const [first, second] = deck.addCards([
    {
      type: "PointForceLoad",
      fields: [10, 1, 0, 100, 1, 0, 0],
      comment: "first",
    },
    {
      type: "PointForceLoad",
      fields: [10, 2, 0, 200, 0, 1, 0],
      comment: "second",
    },
  ]);
  assert.notEqual(first.key, second.key);
  deck.setCardValues(second, { magnitude: 250 });
  assert.equal(deck.cardByKey(first).fields[3], "100.");
  assert.equal(deck.cardByKey(second).fields[3], "250.");
  const before = deck.toModelText();
  assert.throws(
    () => deck.setCardValues(second, { magnitude: 400, typo: 1 }),
    /typo/,
  );
  assert.equal(deck.toModelText(), before);
  deck.setCardComment(second, "updated");
  deck.removeCardRef(first);
  deck.compact();
  assert.equal(deck.cardByKey(first), undefined);
  assert.equal(deck.cardByKey(second).comment, "updated");
  assert.equal(deck.cardByKey(second).key, second.key);
  deck.delete();
});

test("nested groups, sections and matrix forms use the compiled card schema", () => {
  const deck = oor.createModel();
  const rigid = deck.addCard({
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
  assert.match(deck.cardByKey(rigid).text, /ThermalExpansion/);
  deck.addCard({
    type: "DirectMatrix",
    form: "header",
    values: {
      entryName: "K",
      columnIndex: 0,
      matrixForm: 2,
      inputType: 2,
      rowCount: 2,
      columnCount: 1,
    },
  });
  deck.addCard({
    type: "DirectMatrix",
    values: { entryName: "K", columnIndex: 1, firstRow: 1, values: [1.5, 2.5] },
  });
  assert.equal(deck.cardCounts().DirectMatrix, 2);
  const copy = oor.cardSchema("Node");
  copy.fields[0].name = "broken";
  assert.equal(oor.cardSchema("Node").fields[0].name, "id");
  deck.delete();
});

test("case handles retain identity, inherit commands and detect stale sections", () => {
  const deck = oor.createModel();
  const global = deck.caseControl.global;
  global.set("SinglePointConstrSelection", 7);
  global.set("DisplacementOutput", "All");
  const section = deck.caseControl.addSubcase(1);
  section.set("StaticLoadSelection", 10);
  const step = section.addStep(1);
  assert.equal(step.effective("StaticLoadSelection").value, 10);
  section.setSet(9, [1, "Through", 4]);
  section.setParameter("PostprocessingFile", -1);
  section.set("DisplacementOutput", 9, { options: ["PrintListing"] });
  assert.equal(step.effectiveOutputs("DisplacementOutput")[0].value, 9);
  assert.equal(section.remove("DisplacementOutput"), true);
  assert.equal(step.effectiveOutputs("DisplacementOutput")[0].value, "All");
  const before = deck.toModelText();
  assert.throws(
    () =>
      section.setCommands([
        { name: "StaticLoadSelection", value: 20 },
        { name: "StaticLoadSelection", value: "bad" },
      ]),
    /integer/,
  );
  assert.equal(deck.toModelText(), before);
  assert.throws(() => deck.caseControl.addSubcase(1), /already used/);
  section.renumber(2);
  assert.equal(section.snapshot().id, 2);
  const stamp = oor.modelVersions(deck).control;
  section.set("StaticLoadSelection", 20);
  assert.notEqual(oor.modelVersions(deck).control, stamp);
  assert.equal(section.removeSection(), true);
  assert.throws(() => step.snapshot(), /no longer exists/);
  global.set("TitleText", "Still valid");
  deck.caseControl.replace([
    { name: "SubcaseStart", value: 3 },
    { name: "StaticLoadSelection", value: 10 },
  ]);
  assert.throws(() => global.snapshot(), /no longer exists/);
  assert.equal(deck.caseControl.sections()[1].id, 3);
  deck.delete();
});

test("new includes and independent part IDs survive model output and root-handle deletion", () => {
  const deck = oor.createModel();
  deck.addInclude("mesh/nodes.oos", [
    { type: "Node", values: { id: 1, position1: 1 } },
  ]);
  const part = deck.addPart({ id: 5 });
  part.addCard({ type: "Node", values: { id: 1, position1: 99 } });
  deck.setExecutive([{ name: "SolutionSequence", fields: ["LinearStatics"] }]);
  const before = deck.toModelText();
  assert.throws(
    () => deck.setExecutive([{ name: "UnknownStatement", value: 1 }]),
    /not a statement/,
  );
  assert.equal(deck.toModelText(), before);
  deck.writeModel("/work/assembly.oos", { splitIncludes: true });
  assert.match(
    oor.readFile("/work/mesh/nodes.oos", { encoding: "utf8" }),
    /Node;1;/,
  );
  assert.equal(deck.parts()[0].id, 5);
  deck.delete();
  assert.equal(part.nodes[1].xyz[0], 99);
  part.delete();
});

test("catalogues describe control inputs and full case replacement validates before changing keys", () => {
  assert.equal(
    oor.solutionCatalogue().find((entry) => entry.name === "LinearStatics")
      .number,
    101,
  );
  assert.equal(
    oor
      .caseCommandCatalogue()
      .find((entry) => entry.name === "StaticLoadSelection").shape,
    "integer",
  );
  const deck = oor.createModel();
  const global = deck.caseControl.global;
  global.set("TitleText", "Retained after failure");
  const before = deck.toModelText();
  assert.throws(
    () =>
      deck.caseControl.replace([
        { name: "SubcaseStart", value: 1 },
        { name: "SubcaseStart", value: 1 },
      ]),
    /unique/,
  );
  assert.equal(deck.toModelText(), before);
  assert.throws(
    () => deck.caseControl.replace([{ name: "StepStart", value: 1 }]),
    /nesting/,
  );
  assert.equal(global.snapshot().entries[0].value, "Retained after failure");
  const node = deck.addCard({ type: "Node", values: { id: 1, position1: 2 } });
  deck.setCardValues(node, { position1: -0 });
  assert.ok(Object.is(deck.cardByKey(node).values.xyz[0], -0));
  deck.updateCard(node, { values: { id: 1 }, comment: "replaced comment" });
  assert.equal(deck.cardByKey(node).comment, "replaced comment");
  deck.delete();
});

test("model copies share initial data and then edit cards and controls independently", () => {
  const deck = oor.createModel();
  const node = deck.addCard({ type: "Node", values: { id: 1, position1: 1 } });
  deck.caseControl.global.set("TitleText", "Original");
  const copy = deck.copy();
  copy.setCardValues(node, { position1: 2 });
  copy.caseControl.global.set("TitleText", "Copy");
  assert.equal(deck.cardByKey(node).values.xyz[0], 1);
  assert.equal(copy.cardByKey(node).values.xyz[0], 2);
  assert.equal(deck.caseControl.global.snapshot().entries[0].value, "Original");
  assert.equal(copy.caseControl.global.snapshot().entries[0].value, "Copy");
  copy.delete();
  deck.delete();
});

test("singular construction refuses expanding records atomically and batch construction returns every entity", () => {
  const deck = oor.createModel();
  const before = deck.toModelText();
  assert.throws(
    () => deck.addCard({ type: "ScalarPoint", values: { ids: [1, 2, 3] } }),
    /addCards/,
  );
  assert.equal(deck.toModelText(), before);
  const added = deck.addCards([
    { type: "ScalarPoint", values: { ids: [1, 2, 3] } },
  ]);
  assert.deepEqual(
    added.map((card) => card.id),
    [1, 2, 3],
  );
  assert.equal(deck.cardCount(), 3);
  deck.delete();
});
