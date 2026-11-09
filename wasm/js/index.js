// oor-reader: reads and writes .oos model files and native HDF5 model/results archives,
// writes them as an Excel workbook, and runs the named queries of the library, in the browser or in Node.js.
//
//   import { loadOorReader } from "oor-reader";
//   const oor = await loadOorReader();
//   oor.writeFile("/work/plate.oos", modelText);
//   const deck = oor.readModel("/work/plate.oos");
//   const nodes = oor.runQuery("nodesTable", { deck }).columns();
//
// Files live in an in-memory file system: put the files to read in it with `writeFile` (a deck and the files it
// includes), and take the files written out of it with `readFile`.

import createOorReaderCore from "./oor_reader_core.js";

// True when the JavaScript runtime runs 64-bit WebAssembly (Memory64), which the module needs: the smallest module that
// declares a 64-bit memory (magic number, version, then a memory section with the memory64 flag) must validate.
export function supportsMemory64() {
  const module = new Uint8Array([0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x05, 0x03, 0x01, 0x04, 0x01]);
  try {
    return WebAssembly.validate(module);
  } catch {
    return false;
  }
}

// Loads the WebAssembly module. `options` are passed to Emscripten (for example `locateFile` to say where the .wasm
// file is served from).
export async function loadOorReader(options = {}) {
  if (!supportsMemory64()) {
    throw new Error(
      "oor-reader needs 64-bit WebAssembly (Memory64), which this JavaScript runtime does not support: " +
        "use Node.js 24 or later, or a browser that supports Memory64.");
  }
  const core = await createOorReaderCore(options);
  addCardAccess(core.Deck.prototype);
  return new OorReader(core);
}

// The categories of cards reached by ID on a deck (deck.nodes[11], deck.elements[10], ...), and the computed values a
// card of the category gets on its own.
const CARD_CATEGORIES = {
  nodes: { source: "nodes", computed: ["xyzGlobal"] },
  elements: { source: "elements", computed: [] },
  properties: { source: "properties", computed: [] },
  materials: { source: "materials", computed: [] },
  coords: { source: "coordinates", computed: [] },
};
const CARD_KEYS = ["type", "id", "values", "fields", "text", "comment"];

// A card as JavaScript code reads it: { type, id, values, fields, text, comment }, and each named value also as a
// property of its own (card.xyz is card.values.xyz) unless its name is one of those six.
function cardFromView(view) {
  const card = { ...view };
  for (const [name, value] of Object.entries(view.values)) {
    if (!CARD_KEYS.includes(name)) card[name] = value;
  }
  return card;
}

// The card, whose properties can also be assigned: card.t = 2 sets the field the reader of the card type names T
// (deck.cardFieldNames lists those names) and reads the card again, so every property shows the new state.
function cardObject(deck, view, computed) {
  if (view === null) return undefined;
  const state = { card: cardFromView(view) };
  return new Proxy(state, {
    get: (target, property) => target.card[property],
    has: (target, property) => property in target.card,
    ownKeys: (target) => Reflect.ownKeys(target.card),
    getOwnPropertyDescriptor: (target, property) => {
      const descriptor = Reflect.getOwnPropertyDescriptor(target.card, property);
      return descriptor === undefined ? undefined : { ...descriptor, configurable: true };
    },
    set(target, property, value) {
      if (typeof property !== "string" || CARD_KEYS.includes(property)) {
        throw new TypeError(`the property '${String(property)}' of a card cannot be assigned`);
      }
      if (Array.isArray(value) || ArrayBuffer.isView(value)) {
        throw new TypeError(
          `a card field takes one value: set the fields one by one (card.x1 = 0, card.x2 = 1) ` +
            `or use deck.setCardField(type, id, field, text)`,
        );
      }
      const { type, id } = target.card;
      deck.setCardValue(type, id, property, value === null || value === undefined ? "" : String(value));
      target.card = cardFromView(deck.card(type, id, computed));
      return true;
    },
  });
}

// The cards of a category by ID: view[11] is the card 11 (undefined when there is none); view.ids() lists the IDs.
function cardsById(deck, { source, computed }) {
  const methods = {
    get: (id) => cardObject(deck, deck.card(source, Number(id), computed), computed),
    has: (id) => deck.card(source, Number(id), []) !== null,
    ids: () => Array.from(deck.cardIds(source)),
  };
  return new Proxy(methods, {
    get(target, property) {
      if (typeof property === "string" && /^-?\d+$/.test(property)) return target.get(property);
      return target[property];
    },
  });
}

// Adds to the Deck class of the module: deck.nodes, deck.elements, deck.properties, deck.materials, deck.coords (cards
// by ID) and deck.cardById(source, id), for any category or card type ("ShellProp", "SinglePointConstrList").
function addCardAccess(prototype) {
  for (const [name, category] of Object.entries(CARD_CATEGORIES)) {
    Object.defineProperty(prototype, name, {
      get() {
        return cardsById(this, category);
      },
    });
  }
  prototype.cardById = function cardById(source, id, computed = []) {
    return cardObject(this, this.card(source, id, computed), computed);
  };
}

export class OorReader {
  constructor(core) {
    this.core = core;
  }

  // The version of the library.
  version() {
    return this.core.version();
  }

  // ---- files of the in-memory file system

  // Writes a file (text or bytes), making its folders.
  writeFile(path, content) {
    const folder = path.substring(0, path.lastIndexOf("/"));
    if (folder) this.core.FS.mkdirTree(folder);
    this.core.FS.writeFile(path, content);
  }

  // Reads a file: bytes (Uint8Array), or text with { encoding: "utf8" }.
  readFile(path, options = {}) {
    return this.core.FS.readFile(path, options);
  }

  removeFile(path) {
    this.core.FS.unlink(path);
  }

  // ---- decks

  // Reads .oos records and their Include files, or the typed model of a native .oorh5 archive.
  readModel(path, options = {}) {
    return this.core.Deck.readModel(path, { strict: options.strict ?? false });
  }

  // ---- results

  // Opens a result file: an HDF5 results archive, recognised by its content.
  openResults(path) {
    return this.core.Results.open(path);
  }

  // Results made by a program (a solver, a computation in the page): one object a table, checked against the result
  // catalogue. See MemoryTable in index.d.ts for the members of a table.
  createResults(tables, label = "memory") {
    return this.core.Results.fromTables(label, tables);
  }

  // ---- queries

  // Runs a named query. `deck` and `results` are what the query reads (either may be left out when the query does not
  // need it); `args` are the arguments of the query, by name, as plain JavaScript values.
  runQuery(name, { deck = null, results = null, args = {} } = {}) {
    return this.core.runQuery(name, deck, results, JSON.stringify(args));
  }

  // Owned byte packages for the solver's design contract; no pointers cross module memories.
  optimizationTables(deck) {
    const names = ["designVariablesTable", "designLinksTable", "designRelationsTable", "designConstantsTable",
      "shapeVariablesTable", "designEquationSymbols", "designEquationInstructions", "designResponsesTable",
      "designResponseArgumentsTable", "designConstraintsTable", "designConstraintSetsTable", "designSubcasesTable",
      "designScreeningTable"];
    const tables = {};
    for (const name of names) {
      const table = this.runQuery(name, { deck });
      try { tables[name] = table.toPackage(); }
      finally { table.delete(); }
    }
    return tables;
  }

  designVariables(deck) { return this.core.designVariables(deck); }

  // Explicit IDs in solver x order; writes the final point atomically and updates XINIT by default.
  applyDesignPoint(deck, ids, values, options = {}) {
    return this.core.applyDesignPoint(deck, ids, values, options);
  }

  // Every named query, with its description and parameters.
  queryCatalogue() {
    return this.core.queryCatalogue();
  }

  // Writes query tables to an HDF5 file (the custom outputs): tables is [{ name, table }].
  writeQueryTables(path, tables, compressionLevel = 4) {
    this.core.writeQueryTables(path, tables, compressionLevel);
  }

  // ---- card tables (what a program that computes on the model reads, such as the oos-solver package)

  // The cards of one type ("ShellQuad4", "ShellProp"), the grid points ("nodes") or the other points ("points"), sorted by
  // ID, with the columns asked for (every stored column when `columns` is left out). Gives a QueryTable; its
  // toPackage() hands it to another WebAssembly module with its exact Arrow layout.
  cardTable(deck, source, columns = null) {
    return this.core.cardTable(deck, source, columns);
  }

  // The columns a card table can give: { stored: [...], computed: [...] }.
  cardTableColumns(source) {
    return this.core.cardTableColumns(source);
  }

  // The card types of the deck that have cards, in a category: "elements", "properties", "materials", "methods",
  // "loads", "constraints", "coordinates", "nodes" or "points".
  cardTypesOf(deck, category) {
    return this.core.cardTypesOf(deck, category);
  }

  // For every grid point (the rows of cardTable(deck, "nodes")), the 3 x 3 rotation from its displacement system to
  // the basic system, row-major: a Float64Array of 9 values per grid point.
  displacementRotations(deck) {
    return this.core.displacementRotations(deck);
  }

  // The version counters of the model: { structure, content }.
  modelVersions(deck) {
    return this.core.modelVersions(deck);
  }
}
