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

export interface ArchiveWriteOptions {
  compressionLevel?: number; shuffle?: boolean; strict?: boolean;
  title?: string; subtitle?: string; label?: string;
}

export type CardScalar = number | string | null | { real: number };
// Group values are arrays of objects; a one-field group also accepts an array of scalars.
export interface CardData {
  [name: string]: CardScalar | CardData | readonly (CardScalar | CardData)[];
}
export type CardContent = { comment?: string; form?: string } & (
  { fields: ArrayLike<CardScalar>; values?: never } | { values: CardData; fields?: never }
);
export type CardDefinition = CardContent & { type: string };
// A key identifies a card within its model/part, independently of its editable ID or set ID.
export interface CardRef { readonly type: string; readonly key: number; readonly id?: number; }
export interface AddedCard extends CardRef { readonly id: number; }
export interface CardBatchResult { type: string; ids: Int32Array; keys: Uint32Array; }
export type CardBatchColumn = CardScalar | ArrayLike<CardScalar>;

export interface LayoutField { name: string; kind: string; slot?: number; offset?: number; default?: string; span?: number; }
export interface LayoutGroup {
  name: string; fields: LayoutField[]; firstSlot?: number; period?: number; fixedCount?: number; countField?: string;
  list?: LayoutGroup;
}
export interface CardSchema {
  fields: LayoutField[]; group?: LayoutGroup; nextGroups?: LayoutGroup[];
  sections?: { name: string; kinds: { key: string; name: string; fields?: LayoutField[]; list?: LayoutGroup }[] };
  forms?: (CardSchema & { name: string; when?: { field?: string; values?: string[]; kinds?: string[]; blankSlot?: number; filledSlot?: number } })[];
  note?: string;
}

export type ControlValue = number | string | null | ArrayLike<number | string>;
export interface CaseCommandDescription {
  name: string; shape: string; variants: string[]; keywords: string[]; sectionKind: string; blockKind: string; inBlock: boolean;
}
export interface ControlRecord {
  name: string;
  // Use fields for arbitrary catalogue records (sets, parameters, section or output-block headers).
  fields?: ArrayLike<ControlValue>;
  value?: ControlValue;
  options?: string[];
  variant?: string;
  comment?: string;
}
export interface CaseEntrySnapshot {
  key: number; name: string; kind: "command" | "output" | "set" | "parameter" | "metadata";
  value: number | string | number[] | Float64Array | null;
  variant: string; options: string[]; number: number; members: string[]; comment: string; loadSequence: number | null;
}
export type CaseSectionKind = "global" | "subcase" | "subcaseCombination" | "symmetrySubcase" | "symmetryCombination" |
  "repeatSubcase" | "step" | "substep" | "auxiliaryCase";
export interface CaseSectionSnapshot {
  key: number; kind: CaseSectionKind; id: number; entries: CaseEntrySnapshot[]; children: CaseSectionSnapshot[];
}
export interface CaseSection {
  readonly key: number;
  snapshot(): CaseSectionSnapshot;
  set(name: string, value?: ControlValue, options?: { options?: string[]; variant?: string; comment?: string }): number;
  setCommands(commands: readonly ControlRecord[]): Uint32Array;
  setSet(id: number, members: ArrayLike<number | string>, comment?: string): number;
  setParameter(name: string, value: ControlValue, comment?: string): number;
  effective(name: string, variant?: string): CaseEntrySnapshot | null;
  effectiveOutputs(name: string): CaseEntrySnapshot[];
  removeEntry(key: number): boolean;
  remove(name: string, variant?: string): boolean;
  addStep(id: number, options?: { commands?: readonly ControlRecord[] }): CaseSection;
  addSubstep(id: number, options?: { commands?: readonly ControlRecord[] }): CaseSection;
  renumber(id: number): void;
  removeSection(): boolean;
}
export interface CaseControl {
  readonly global: CaseSection;
  sections(): CaseSectionSnapshot[];
  section(key: number): CaseSection;
  addSubcase(id: number, options?: { kind?: Exclude<CaseSectionKind, "global" | "step" | "substep" | "auxiliaryCase">;
    commands?: readonly ControlRecord[] }): CaseSection;
  addAuxiliaryCase(): CaseSection;
  // Replaces the complete section from structured records. All previous case handles become stale.
  replace(records: readonly ControlRecord[]): void;
  records(): ControlRecord[];
}
export interface PartDefinition { kind?: string; id?: number; massId?: number; appends?: boolean; }

// A named value of a card: a number, a list (Float64Array), a text, a list of texts, or null when blank.
export type CardValue = number | Float64Array | string | string[] | null;

// A card read by ID. Each named value is also a property of its own (card.xyz), unless its name is one of the seven below.
export interface Card {
  type: string;                       // "Node"
  id: number;
  key: number;
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
  copy(): Deck; // independent copy; column buffers are shared until edited
  // Empty-model construction uses OorReader.createModel(). Each successful insertion returns stable card keys.
  addCard(card: CardDefinition): AddedCard;
  addCards(cards: readonly CardDefinition[]): AddedCard[];
  // Fixed layouts only; scalar columns broadcast. Numeric typed arrays cross the boundary once per column.
  addCardBatch(type: string, columns: Record<string, CardBatchColumn>, options?: { count?: number }): CardBatchResult;
  cardByKey(ref: CardRef, computed?: string[]): Card | undefined;
  updateCard(ref: CardRef, card: CardContent): void;
  setCardValues(ref: CardRef, values: Record<string, CardScalar>): void;
  removeCardRef(ref: CardRef): boolean;
  setCardComment(ref: CardRef, comment: string): void;
  addInclude(path: string, cards: readonly CardDefinition[]): AddedCard[];
  addPart(part: PartDefinition): Deck;
  part(part: PartDefinition): Deck | null;
  parts(): Required<PartDefinition>[];
  readonly caseControl: CaseControl;
  setSolution(solution: string | number): void;
  setExecutive(records: readonly ControlRecord[]): void;
  executiveRecords(): ControlRecord[];
  caseRecords(): ControlRecord[];
  applyDefaults(): void;
  checkReferences(): Diagnostic[];
  compact(): void;
  // A native archive containing the model alone.
  writeArchive(path: string, options?: ArchiveWriteOptions): WriteReport;
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
  addCards(text: string): AddedCard[];                                       // one card or several, all or none
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
  hasModel(): boolean;
  // Loads and attaches the embedded model on demand; null when no model is available.
  model(): Deck | null;
  // Includes the attached model, or preserves the embedded model when rewriting an archive.
  writeArchive(path: string, options?: ArchiveWriteOptions): WriteReport;
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
  createModel(): Deck;
  cardTypes(): string[];
  solutionCatalogue(): { name: string; number: number }[];
  caseCommandCatalogue(): CaseCommandDescription[];
  // A detached schema snapshot from the compiled catalogue. Field names are exact and case-sensitive.
  cardSchema(type: string): CardSchema;
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
  modelVersions(deck: Deck): { structure: number; content: number; control: number };
}

export declare function loadOorReader(options?: Record<string, unknown>): Promise<OorReader>;
// True when the runtime supports 64-bit WebAssembly (Memory64), which the module needs.
export declare function supportsMemory64(): boolean;
