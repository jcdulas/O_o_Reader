// The model-construction adapter. The compiled catalogue supplies names and record order; C++ owns validation,
// identity, storage and atomic edits. No file or whole-model text is generated here.

const own = (object, name) =>
  Object.prototype.hasOwnProperty.call(object, name);
function append(target, values) {
  for (const value of values) target.push(value);
}
const isList = (value) =>
  Array.isArray(value) ||
  (ArrayBuffer.isView(value) && !(value instanceof DataView));

function object(value, label) {
  if (value === null || typeof value !== "object" || isList(value))
    throw new TypeError(`${label} must be an object`);
  return value;
}

function scalar(value, kind) {
  if (value === undefined || value === null) return null;
  if (typeof value === "number") {
    if (kind === "integer" && !Number.isInteger(value))
      throw new TypeError("an integer field needs an integer");
    if (
      !Number.isFinite(value) ||
      (kind !== "real" &&
        Number.isInteger(value) &&
        !Number.isSafeInteger(value))
    ) {
      throw new TypeError(
        "a numeric field must be finite and an integer must be safe",
      );
    }
    return kind === "real" || (kind !== "integer" && Object.is(value, -0))
      ? { real: value }
      : value;
  }
  if (typeof value === "string" && !/[\r\n]/.test(value)) {
    const text = value.trim();
    if (kind === "integer" && text !== "" && !/^[+-]?\d+$/.test(text))
      throw new TypeError("an integer field needs an integer");
    return kind === "real" && /^[+-]?\d+$/.test(text) ? `${text}.` : value;
  }
  if (
    typeof value === "object" &&
    Object.keys(value).length === 1 &&
    own(value, "real") &&
    Number.isFinite(value.real)
  ) {
    if (kind === "integer")
      throw new TypeError(
        "an integer field cannot take an explicit real value",
      );
    return { real: value.real };
  }
  throw new TypeError("a field is a number, string, null or {real: number}");
}

function numeric(value) {
  return value && typeof value === "object" && own(value, "real")
    ? value.real
    : value;
}

function chooseForm(layout, data, requested) {
  if (!layout.forms) {
    if (requested !== undefined) throw new Error("this card has no forms");
    return layout;
  }
  if (requested !== undefined) {
    const form = layout.forms.find((candidate) => candidate.name === requested);
    if (!form) throw new Error(`unknown form '${requested}'`);
    return form;
  }
  for (const form of layout.forms) {
    const when = form.when;
    if (!when) return form;
    if (!when.field) continue; // internal slot-only forms are never selected by a compact record
    let value = data[when.field];
    const group = form.group;
    if (value === undefined && group && data[group.name]?.length) {
      const first = data[group.name][0];
      value =
        group.fields.length === 1 &&
        group.fields[0].name === when.field &&
        typeof first !== "object"
          ? first
          : first?.[when.field];
    }
    if (value === undefined && form.sections?.name === when.field)
      value = data[when.field]?.[0]?.key;
    const number = numeric(value);
    if (
      when.values?.some(
        (word) =>
          String(word).toLowerCase() === String(number ?? "").toLowerCase() ||
          (word !== "" &&
            number !== undefined &&
            number !== null &&
            Number.isFinite(Number(word)) &&
            Number(word) === Number(number)),
      )
    )
      return form;
    let kind =
      value == null
        ? "blank"
        : typeof number === "number"
          ? typeof value === "object" || !Number.isInteger(number)
            ? "real"
            : "integer"
          : /^[+-]?\d+$/.test(number)
            ? "integer"
            : /^[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?$/.test(number)
              ? "real"
              : "text";
    if (
      kind === "integer" &&
      typeof number === "number" &&
      form.fields?.find((field) => field.name === when.field)?.kind === "real"
    )
      kind = "real";
    if (when.kinds?.includes(kind)) return form;
  }
  throw new Error(
    "no card form matches these values; select a form explicitly",
  );
}

// Names are checked at every level, so a misspelled property never silently becomes a blank field.
function pack(layout, source, requested) {
  const data = object(source, "card values");
  layout = chooseForm(layout, data, requested);
  const allowed = new Set((layout.fields ?? []).map((field) => field.name));
  const fields = (layout.fields ?? []).map((field) =>
    scalar(data[field.name], field.kind),
  );
  const groups = [
    ...(layout.group ? [layout.group] : []),
    ...(layout.nextGroups ?? []),
  ];
  groups.forEach((group, index) => {
    allowed.add(group.name);
    const rows = data[group.name] ?? [];
    if (!isList(rows)) throw new TypeError(`'${group.name}' must be an array`);
    if (group.fixedCount && rows.length !== group.fixedCount)
      throw new Error(`'${group.name}' needs ${group.fixedCount} items`);
    if (group.countField) {
      const countIndex = (layout.fields ?? []).findIndex(
        (field) => field.name === group.countField,
      );
      if (countIndex < 0)
        throw new Error(
          `count field '${group.countField}' needs compact fields for this layout`,
        );
      if (
        data[group.countField] != null &&
        Number(data[group.countField]) !== rows.length
      )
        throw new Error(
          `'${group.countField}' differs from '${group.name}' length`,
        );
      fields[countIndex] = rows.length;
    } else if (
      !group.fixedCount &&
      (index < groups.length - 1 || layout.sections)
    ) {
      fields.push(rows.length);
    }
    for (const row of rows) append(fields, packGroup(group, row));
  });
  if (layout.sections) {
    allowed.add(layout.sections.name);
    const sections = data[layout.sections.name] ?? [];
    if (!Array.isArray(sections))
      throw new TypeError(`'${layout.sections.name}' must be an array`);
    for (const section of sections) {
      object(section, "card section");
      const kind = layout.sections.kinds.find(
        (candidate) => candidate.key === section.key,
      );
      if (!kind) throw new Error(`unknown section key '${section.key}'`);
      fields.push(kind.key);
      const values = { ...section };
      delete values.key;
      append(fields, packGroup(kind, values));
    }
  }
  for (const name of Object.keys(data))
    if (!allowed.has(name))
      throw new Error(`unknown card field or group '${name}'`);
  return fields;
}

function packGroup(group, source) {
  const fields = group.fields ?? [];
  const data =
    fields.length === 1 &&
    !group.list &&
    (source === null || typeof source !== "object")
      ? { [fields[0].name]: source }
      : object(source, "group item");
  const allowed = new Set(fields.map((field) => field.name));
  const values = fields.map((field) => scalar(data[field.name], field.kind));
  if (group.list) {
    allowed.add(group.list.name);
    const rows = data[group.list.name] ?? [];
    if (!isList(rows))
      throw new TypeError(`'${group.list.name}' must be an array`);
    values.push(rows.length);
    for (const row of rows) append(values, packGroup(group.list, row));
  }
  for (const name of Object.keys(data))
    if (!allowed.has(name)) throw new Error(`unknown group field '${name}'`);
  return values;
}

function controlField(value) {
  if (isList(value)) return Array.from(value, controlField).join(",");
  if (value === undefined || value === null) return "";
  if (
    typeof value === "number" &&
    (!Number.isFinite(value) ||
      (Number.isInteger(value) && !Number.isSafeInteger(value)))
  ) {
    throw new TypeError("a control value must be finite and safe");
  }
  if (
    !["number", "string"].includes(typeof value) ||
    /[\r\n]/.test(String(value))
  )
    throw new TypeError("invalid control field");
  return String(value);
}

function controlRecord(record) {
  object(record, "control record");
  if (typeof record.name !== "string" || !record.name)
    throw new TypeError("a control record needs a name");
  const allowed = new Set([
    "name",
    "fields",
    "value",
    "options",
    "variant",
    "comment",
  ]);
  for (const key of Object.keys(record))
    if (!allowed.has(key)) throw new Error(`unknown control property '${key}'`);
  if (
    record.fields !== undefined &&
    ["value", "options", "variant"].some((key) => own(record, key))
  )
    throw new Error("use fields or value/options/variant");
  if (record.fields !== undefined && !isList(record.fields))
    throw new TypeError("control fields must be an array");
  if (record.options !== undefined && !Array.isArray(record.options))
    throw new TypeError("control options must be an array");
  return {
    name: record.name,
    fields:
      record.fields !== undefined
        ? Array.from(record.fields, controlField)
        : [
            controlField(record.value),
            ...(record.variant ? [controlField(record.variant)] : []),
            ...(record.options ?? []).map(controlField),
          ],
    comment: record.comment ?? "",
  };
}

class CaseSection {
  constructor(deck, key) {
    this.deck = deck;
    this.key = key;
  }
  snapshot() {
    return this.deck._caseSection(this.key);
  }
  set(name, value = null, options = {}) {
    return this.setCommands([{ name, value, ...options }])[0];
  }
  setCommands(commands) {
    if (!Array.isArray(commands))
      throw new TypeError("commands must be an array");
    return this.deck._setCaseCommands(this.key, commands.map(controlRecord));
  }
  setSet(id, members, comment = "") {
    if (!isList(members)) throw new TypeError("set members must be an array");
    return this.setCommands([
      { name: "SetDefinition", fields: [id, ...members], comment },
    ])[0];
  }
  setParameter(name, value, comment = "") {
    return this.setCommands([
      {
        name: "SubcaseParameter",
        fields: [name, controlField(value)],
        comment,
      },
    ])[0];
  }
  effective(name, variant = "") {
    return this.deck._effectiveCaseCommand(this.key, name, variant);
  }
  effectiveOutputs(name) {
    return this.deck._effectiveCaseOutputs(this.key, name);
  }
  removeEntry(key) {
    if (!this.snapshot().entries.some((entry) => entry.key === key))
      return false;
    return this.deck._removeCaseEntry(key);
  }
  remove(name, variant = "") {
    const entries = this.snapshot().entries.filter(
      (entry) => entry.name === name && entry.variant === variant,
    );
    let removed = false;
    for (const entry of entries)
      removed = this.deck._removeCaseEntry(entry.key) || removed;
    return removed;
  }
  addStep(id, options = {}) {
    return this.addChild("step", id, options);
  }
  addSubstep(id, options = {}) {
    return this.addChild("substep", id, options);
  }
  addChild(kind, id, { commands = [] } = {}) {
    const key = this.deck._addCaseSection(
      kind,
      id,
      this.key,
      commands.map(controlRecord),
    );
    return new CaseSection(this.deck, key);
  }
  renumber(id) {
    this.deck._renumberCaseSection(this.key, id);
  }
  removeSection() {
    return this.deck._removeCaseSection(this.key);
  }
}

class CaseControl {
  constructor(deck) {
    this.deck = deck;
  }
  get global() {
    return new CaseSection(this.deck, this.deck._caseGlobalKey());
  }
  sections() {
    return this.deck._caseSections();
  }
  replace(records) {
    this.deck._setCaseControl(records.map(controlRecord));
  }
  records() {
    return this.deck.caseRecords();
  }
  section(key) {
    this.deck._caseSection(key);
    return new CaseSection(this.deck, key);
  }
  addSubcase(id, { kind = "subcase", commands = [] } = {}) {
    return new CaseSection(
      this.deck,
      this.deck._addCaseSection(kind, id, 0, commands.map(controlRecord)),
    );
  }
  addAuxiliaryCase() {
    return new CaseSection(
      this.deck,
      this.deck._addCaseSection("auxiliaryCase", 0, 0, []),
    );
  }
}

export function installModelEditing(core) {
  let catalogue;
  const schemas = () => (catalogue ??= JSON.parse(core.cardLayouts()).cards);
  const schema = (type) => {
    if (!own(schemas(), type)) throw new Error(`unknown card type '${type}'`);
    return schemas()[type];
  };
  const input = (card) => {
    object(card, "card");
    const allowed = new Set(["type", "fields", "values", "form", "comment"]);
    for (const name of Object.keys(card))
      if (!allowed.has(name))
        throw new Error(`unknown card input property '${name}'`);
    const layout = schema(card.type);
    if ((card.fields === undefined) === (card.values === undefined))
      throw new Error("a card needs either fields or values");
    if (card.fields !== undefined && !isList(card.fields))
      throw new TypeError("card fields must be an array");
    if (own(card, "comment") && typeof card.comment !== "string")
      throw new TypeError("a comment must be a string");
    return {
      type: card.type,
      fields:
        card.fields !== undefined
          ? Array.from(card.fields, (value, i) =>
              scalar(
                value,
                layout.forms ? undefined : layout.fields?.[i]?.kind,
              ),
            )
          : pack(layout, card.values, card.form),
      ...(own(card, "comment") ? { comment: card.comment } : {}),
      ...(card.form === undefined ? {} : { form: card.form }),
    };
  };
  const prototype = core.Deck.prototype;
  const addText = prototype.addCards;
  prototype.addCards = function (cards) {
    if (typeof cards === "string") return addText.call(this, cards);
    if (!Array.isArray(cards))
      throw new TypeError("cards must be an array or model records");
    return this._insertCards(cards.map(input));
  };
  prototype.addCard = function (card) {
    return this._insertCard(input(card));
  };
  prototype.addCardBatch = function (type, columns, { count } = {}) {
    const layout = schema(type);
    object(columns, "batch columns");
    if (layout.forms || layout.group || layout.sections)
      throw new Error(
        "column batches require a fixed layout; use addCards for groups, sections and forms",
      );
    const used = new Set();
    const inputColumns = Object.entries(columns).map(([name, data]) => {
      const field = /^\d+$/.test(name)
        ? Number(name)
        : 1 + layout.fields.findIndex((entry) => entry.name === name);
      if (field < 1 || field > layout.fields.length || used.has(field))
        throw new Error(`invalid or repeated batch field '${name}'`);
      used.add(field);
      const broadcast = !isList(data);
      if (!broadcast) {
        count ??= data.length;
        if (data.length !== count)
          throw new Error("batch column lengths differ");
      } else data = scalar(data, layout.fields[field - 1].kind);
      return {
        field,
        data,
        broadcast,
        real: layout.fields[field - 1].kind === "real",
      };
    });
    if (!Number.isInteger(count) || count < 0 || count > 0xffffffff)
      throw new Error("a batch needs a count or at least one array column");
    return this._insertCardBatch(type, count, inputColumns);
  };
  prototype.cardByKey = function (ref, computed = []) {
    return this._cardByKey(ref.type, ref.key, computed) ?? undefined;
  };
  prototype.updateCard = function (ref, card) {
    if (card.type !== undefined && card.type !== ref.type)
      throw new Error("a replacement must keep the card type");
    this._updateCard(ref.type, ref.key, input({ ...card, type: ref.type }));
  };
  prototype.setCardValues = function (ref, values) {
    object(values, "card values");
    const fields = schema(ref.type).fields ?? [];
    const converted = Object.fromEntries(
      Object.entries(values).map(([name, value]) => [
        name,
        scalar(value, fields.find((field) => field.name === name)?.kind),
      ]),
    );
    this._setCardValues(ref.type, ref.key, converted);
  };
  prototype.removeCardRef = function (ref) {
    return this._removeCard(ref.type, ref.key);
  };
  prototype.setCardComment = function (ref, comment) {
    this._setCardComment(ref.type, ref.key, comment);
  };
  prototype.addInclude = function (path, cards) {
    return this._appendInclude(path, cards.map(input));
  };
  prototype.setSolution = function (solution) {
    this._setSolution(controlField(solution));
  };
  prototype.setExecutive = function (records) {
    this._setExecutive(records.map(controlRecord));
  };
  Object.defineProperty(prototype, "caseControl", {
    get() {
      return new CaseControl(this);
    },
  });
  prototype.addPart = function ({
    kind = "SuperelementPart",
    id = 0,
    massId = 0,
    appends = false,
  }) {
    return this._addPart(kind, id, massId, appends);
  };
  prototype.part = function ({
    kind = "SuperelementPart",
    id = 0,
    massId = 0,
  }) {
    return this._part(kind, id, massId);
  };
  return {
    cardSchema: (type) => structuredClone(schema(type)),
    cardTypes: () => Object.keys(schemas()),
  };
}
