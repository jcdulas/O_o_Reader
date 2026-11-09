// Types of the oor-reader package (see index.js).

export interface Diagnostic {
  severity: "info" | "warning" | "error";
  code: string;
  message: string;
  location: string;
}

export interface DeckWriteOptions {
  splitIncludes?: boolean;
  headerComment?: string;
}

// A named value of a card: a number, a list (Float64Array), a text, a list of texts, or null when blank.
export type CardValue = number | Float64Array | string | string[] | null;

// A card read by ID. Each named value is also a property of its own (card.xyz), unless its name is one of the six below.
export interface Card {
  type: string;                       // "Node"
  id: number;
  values: Record<string, CardValue>;  // the stored fields of the card type, and the computed values asked for
  fields: string[];                   // the fields as written, field 1 first ("" for a blank field)
  text: string;                       // the card as one model record
  comment: string;                    // the comment lines above the card
  [name: string]: unknown;
}

// The cards of a category by ID: deck.nodes[11] (undefined when there is none).
export interface CardsById {
  [id: number]: Card | undefined;
  get(id: number): Card | undefined;
  has(id: number): boolean;
  ids(): number[];
}

export interface Deck {
  // Writes an O_o model file; returns any content that could not be carried, one description per item.
  writeModel(path: string, options?: DeckWriteOptions): string[];
  // The model file text (one file).
  toModelText(options?: DeckWriteOptions): string;
  cardCounts(): Record<string, number>;
  cardCount(): number;
  diagnostics(): Diagnostic[];

  // ---- cards by ID ("source": "nodes", "elements", "properties", "materials", "coordinates" or a card type)
  readonly nodes: CardsById;      // each node also has xyzGlobal, its position in the basic system
  readonly elements: CardsById;
  readonly properties: CardsById;
  readonly materials: CardsById;
  readonly coords: CardsById;
  cardById(source: string, id: number, computed?: string[]): Card | undefined;
  cardIds(source: string): Int32Array;
  cardFields(source: string, id: number): string[];

  // ---- editing: an edited card keeps its place, its comment and its link to attached results; a refused edit (a
  // field the card cannot hold, an ID already taken) changes nothing and throws an Error that says why
  setCardField(source: string, id: number, field: number, value: string): void; // field 1 is the first after the name
  setCardFields(source: string, id: number, fields: string[]): void;
  cardFieldNames(source: string, id: number): { field: number; name: string }[]; // [{ field: 3, name: "thickness" }, ...]
  setCardValue(source: string, id: number, name: string, value: string): void;   // a field by its exact neutral name
  replaceCard(source: string, id: number, text: string): void;                // one model record, same card type
  addCards(text: string): { type: string; id: number }[];                      // one card or several, all or none
  removeCard(source: string, id: number): boolean;

  delete(): void; // frees the C++ object
}

export interface TableDescription {
  index: number;
  kind: string;
  variant: string;
  elementType: string;
  layout: string;
  subcase: number;
  analysis: string;
  stepKind: string;
  steps: Float64Array;
  eigenvalues: Float64Array;
  cycles: Float64Array;
  rowCount: number;
  components: string[];
  valueType: "float32" | "float64" | "complex64" | "complex128";
  tableName: string;
  title: string;
  subtitle: string;
  label: string;
}

export interface TableSelection {
  stepIndices?: number[];
  components?: string[];
  ids?: number[];
  elementIds?: number[];
}

export interface TableValues {
  shape: number[]; // [steps, rows, components] (+ [2] for complex values)
  valueType: string;
  components: string[];
  steps: Float64Array;
  ids: Int32Array;
  nodeIds: Int32Array;
  layers: Int32Array;
  elementIds: Int32Array;
  texts: Record<string, string[]>;
  values: Float32Array | Float64Array;
}

export interface WriteReport {
  path: string;
  format: string;
  finished: boolean;
  bytes: number;
  written: { kind: string; container: string; subcase: number; location: string; steps: number; rows: number }[];
  skipped: { kind: string; container: string; subcase: number; reason: string; message: string }[];
}

// A table made by a program, for createResults. The values are (steps, rows, components) in C order; a complex table
// has the real and imaginary parts of each value one after the other. A Float32Array keeps single precision.
export interface MemoryTable {
  kind: string;                // "displacements", "singlePointConstrForces", "stress", "forces", "eigenvalues", ...
  elementType?: string;        // the element type of an element table: "ShellQuad4", "Bar2", ...
  variant?: string;            // "" (default), "psd", "rms", ...
  subcase?: number;            // 1 by default
  analysisCode?: number;       // taken from the step kind when left out
  stepKind?: "none" | "time" | "frequency" | "mode" | "loadFactor";
  steps?: number[];            // one value a step; left out: one step with no value (a static table)
  eigenvalues?: number[];
  cycles?: number[];
  ids: ArrayLike<number>;      // the grid point or element of each row
  pointTypes?: ArrayLike<number>;
  nodeIds?: ArrayLike<number>;
  elementIds?: ArrayLike<number>;
  layers?: ArrayLike<number>;
  positions?: ArrayLike<number>;
  texts?: Record<string, string[]>;
  components?: string[];       // left out: every component of the kind, in the catalogue's order
  values: Float32Array | Float64Array | number[];
  complex?: boolean;
  title?: string;
  subtitle?: string;
  label?: string;
  tableName?: string;
  units?: string;
}

export interface Results {
  format(): "archive" | "memory";
  metadata(): {
    format: string; path: string; fileSize: number; producer: string; createdAt: string;
    title: string; subtitle: string; label: string; tableCount: number; namedCount: number; complete: boolean;
  };
  diagnostics(): Diagnostic[];
  tables(): TableDescription[];
  read(index: number, selection?: TableSelection): TableValues;
  attach(deck: Deck, strict: boolean): void;
  writeArchive(path: string, options?: {
    compressionLevel?: number; shuffle?: boolean; strict?: boolean; title?: string; subtitle?: string; label?: string;
  }): WriteReport;
  // An Excel workbook (.xlsx) of the tables, for people: an index sheet, one sheet per table.
  writeXlsx(path: string, options?: {
    title?: string; subtitle?: string; label?: string; compressionLevel?: number;
  }): WriteReport;
  delete(): void;
}

export interface QueryColumn {
  name: string;
  kind: "number" | "text" | "numberList" | "textList";
  integer: boolean;
  values: Float64Array | string[];
  offsets?: Float64Array; // lists: row i holds values[offsets[i] .. offsets[i + 1])
  valid?: Uint8Array;     // when the column has nulls: 1 a value, 0 a null
}

export interface QueryTable {
  columnNames(): string[];
  rowCount(): number;
  column(name: string): QueryColumn;
  columns(): Record<string, QueryColumn>;
  toCsv(): string;
  // The table as a table package (docs/table-package.md): bytes that keep its exact Arrow layout, for another module.
  toPackage(): Uint8Array;
  delete(): void;
}

export interface QueryDescription {
  name: string;
  family: string;
  description: string;
  resultQuery: boolean;
  needsModel: boolean;
  parameters: { name: string; default: string; description: string }[];
}

export declare class OorReader {
  version(): string;
  writeFile(path: string, content: string | Uint8Array): void;
  readFile(path: string, options?: { encoding?: "utf8" | "binary" }): Uint8Array | string;
  removeFile(path: string): void;
  readModel(path: string, options?: { strict?: boolean }): Deck;
  openResults(path: string): Results;
  createResults(tables: MemoryTable[], label?: string): Results;
  runQuery(name: string, inputs?: { deck?: Deck | null; results?: Results | null; args?: Record<string, unknown> }): QueryTable;
  optimizationTables(deck: Deck): Record<string, Uint8Array>;
  designVariables(deck: Deck): {
    variables: { id: number; label: string; initialValue: number; lower: number; upper: number;
      moveLimit: number; dependent: boolean }[];
    problems: string[]; notApplied: { card: string; id: number; reason: string }[];
  };
  applyDesignPoint(deck: Deck, ids: number[] | Int32Array, values: number[] | Float64Array,
    options?: { limits?: "error" | "clip" | "ignore"; setInitialValues?: boolean }): {
      variables: Float64Array; changedFields: number; movedGrids: number;
      fields: { relation: number; before: number; requested: number; after: number; limit: number; changed: boolean }[];
    };

  queryCatalogue(): QueryDescription[];
  writeQueryTables(path: string, tables: { name: string; table: QueryTable }[], compressionLevel?: number): void;
  cardTable(deck: Deck, source: string, columns?: string[] | null): QueryTable;
  cardTableColumns(source: string): { stored: string[]; computed: string[] };
  cardTypesOf(deck: Deck, category: "elements" | "properties" | "materials" | "methods" | "loads" | "constraints" |
    "coordinates" | "nodes" | "points"): string[];
  displacementRotations(deck: Deck): Float64Array;
  modelVersions(deck: Deck): { structure: number; content: number };
}

export declare function loadOorReader(options?: Record<string, unknown>): Promise<OorReader>;
// True when the runtime supports 64-bit WebAssembly (Memory64), which the module needs.
export declare function supportsMemory64(): boolean;
