#!/usr/bin/env python3
"""Generates the card layout catalogue, formats/oos-layouts.json.

A card reader reads the fields of a card by slot (FieldReader numbering: slot 1 is the first field after the name, the
slots of a continuation line follow). A model file (.oos) writes a card as one record in a compact order. The layout of
a card type maps the fields of its record to slots; this script makes a first catalogue from what the code does:

  1. the traces of a build configured with -DOOR_TRACE_SLOTS=ON, written while its tests run (trace_slots.sh): every
     slot a card reader looked at, with the field name it gives, and every set of slots a card writer filled;
  2. a reading of the card tables (src/model/cards/**/<Card>Table.cpp) for the slots the tests never reach, the names
     of the fields read as text and the defaults;
  3. the decisions kept by hand: review.json (the kinds of alternative fields) and the files of review/, one per card
     family, each with the cards whose layout was reviewed: a whole layout (fields, group, sections), copied as it is,
     or new names for the fields of the generated layout ({"names": {"field9": "lineKeyword"}}).

From these, for each card type: the fixed fields in slot order (slots no reader or writer uses are padding and get no
field), two fields on one slot when the code reads an integer or a real there under two names (alternatives), and a
repeated group when a field name comes back every `period` slots. A card whose slots cannot be described that way keeps
the rest of its slots, from the first one in doubt, as a group of period 1 (noted in the layout).

Every card type has its whole layout in a review file: the review files are the source of truth, and running this
script again gives the same catalogue whatever the traces. The generated layout is only a proposal for a card type that
has no review entry yet (a new card), to be checked and written into the review file of its family. Each layout is
checked as CardLayouts::fromJson checks it.

Usage: python tools/oos_layouts/generate_layouts.py --traces <folder> [--output formats/oos-layouts.json]
"""

import argparse
import json
import math
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
CARDS_FOLDER = ROOT / "src" / "model" / "cards"
REVIEW = Path(__file__).resolve().parent / "review.json"
REVIEW_FOLDER = Path(__file__).resolve().parent / "review"
LINE_SLOTS = 8
SPAN_TO_END = 1 << 20  # a span of 0: to the end of the card
FORMAT_VERSION = 3

READER_NUMBER_CALLS = {
    "integer": "integer", "requiredInteger": "integer", "optionalInteger": "integer",
    "real": "real", "requiredReal": "real", "optionalReal": "real", "integerOrReal": "number",
}
READER_TEXT_CALLS = {"text", "word", "isBlank", "spanText"}
WRITER_CALLS = {"integer": "i", "integerUnlessDefault": "i", "real": "r", "realUnlessDefault": "r", "text": "t",
                "textUnlessDefault": "t", "spanText": "t", "namedReal": "t"}
# A field name of the code: lowerCamelCase.
FIELD_NAME = re.compile(r"[a-z][A-Za-z0-9]*")
NUMBER_LITERAL = re.compile(r"^-?\d+(\.\d*)?([eE][-+]?\d+)?$")


# ---------------------------------------------------------------------------------------------------------- traces

class Trace:
    """What the traces say about one card type."""

    def __init__(self):
        self.named = defaultdict(set)  # slot -> {(accessor, name)}
        self.looked = set()  # slots looked at as text
        self.spans = set()  # (first, last)
        self.writes = []  # list of {slot: kind}


def read_traces(folder):
    traces = defaultdict(Trace)
    writes = defaultdict(set)
    for path in sorted(Path(folder).glob("trace-*.txt")):
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            parts = line.split("\t")
            if parts[0] == "R" and len(parts) == 5:
                _, card, slot, accessor, name = parts
                if accessor == "text":
                    traces[card].looked.add(int(slot))
                else:
                    traces[card].named[int(slot)].add((accessor, name))
            elif parts[0] == "S" and len(parts) == 4:
                traces[parts[1]].spans.add((int(parts[2]), int(parts[3])))
            elif parts[0] == "W" and len(parts) >= 2:
                writes[parts[1]].add(parts[2] if len(parts) > 2 else "")
    for card, patterns in writes.items():
        for pattern in sorted(patterns):
            slots = {}
            for item in filter(None, pattern.split(",")):
                slot, kind = item.split(":")
                slots[int(slot)] = kind
            traces[card].writes.append(slots)
    return traces


# ---------------------------------------------------------------------------------------------------------- sources

def call_arguments(text, start):
    """The arguments of the call whose '(' is at `start`, split at the top-level commas; and the end index."""
    depth = 0
    arguments, current = [], ""
    index = start
    in_string = False
    while index < len(text):
        character = text[index]
        if in_string:
            current += character
            if character == "\\":
                current += text[index + 1]
                index += 1
            elif character == '"':
                in_string = False
        elif character == '"':
            in_string = True
            current += character
        elif character == "(":
            depth += 1
            if depth > 1:
                current += character
        elif character == ")":
            depth -= 1
            if depth == 0:
                arguments.append(current.strip())
                return arguments, index
            current += character
        elif character == "," and depth == 1:
            arguments.append(current.strip())
            current = ""
        else:
            current += character
        index += 1
    return arguments, index


def evaluate_slot(expression, constants):
    """The slot an expression names: a number, or None, with the variable and offset of a relative one."""
    expression = expression.strip()
    if re.fullmatch(r"\d+", expression):
        return int(expression), None
    if expression in constants:
        return constants[expression], None
    match = re.fullmatch(r"(\w+)\s*\+\s*(\w+)", expression)
    if match:
        left, right = match.groups()
        values = [constants.get(left, int(left) if left.isdigit() else None),
                  constants.get(right, int(right) if right.isdigit() else None)]
        if values[0] is not None and values[1] is not None:
            return values[0] + values[1], None
        if values[1] is not None:
            return None, (left, values[1])
        if values[0] is not None:
            return None, (right, values[0])
    if re.fullmatch(r"\w+", expression):
        return None, (expression, 0)
    return None, None


class Source:
    """What the table source of one card type says."""

    def __init__(self):
        self.fixed = defaultdict(set)  # slot -> {(accessor, name)}
        self.defaults = {}  # name -> default text
        self.text_names = {}  # slot -> name (fields read or written as text)
        self.relative_names = defaultdict(set)  # offset -> {name} (fields at variable + offset)
        self.variable = False  # the reader loops over the fields up to the last one used


def member_name(expression):
    match = re.search(r"\bcard\.(\w+)", expression) or re.search(r"\b\w+\.(\w+)\b(?!\s*\()", expression)
    return match.group(1) if match else None


def default_text(expression):
    expression = expression.strip()
    if NUMBER_LITERAL.match(expression):
        return expression
    match = re.fullmatch(r"card\.(\w+)", expression)
    if match:
        return "same as " + match.group(1)
    return None


def read_source(path):
    source = Source()
    if path is None or not path.exists():
        return source
    text = path.read_text(encoding="utf-8")
    constants = {name: int(value) for name, value in re.findall(r"constexpr\s+(?:int|std::size_t)\s+(\w+)\s*=\s*(\d+)\s*;", text)}
    source.variable = "lastUsedField()" in text or "slotCount()" in text
    lines = text.splitlines()
    offsets = [0]
    for line in lines:
        offsets.append(offsets[-1] + len(line) + 1)

    def line_of(index):
        low, high = 0, len(offsets) - 1
        while low < high:
            middle = (low + high + 1) // 2
            if offsets[middle] <= index:
                low = middle
            else:
                high = middle - 1
        return low

    for match in re.finditer(r"\breader\.(\w+)\(", text):
        call = match.group(1)
        arguments, end = call_arguments(text, match.end() - 1)
        if not arguments or not arguments[0]:
            continue
        slot, relative = evaluate_slot(arguments[0], constants)
        if call in READER_NUMBER_CALLS:
            if len(arguments) < 2 or not arguments[1].startswith('"'):
                continue
            name = arguments[1].strip('"')
            if slot is not None:
                source.fixed[slot].add((READER_NUMBER_CALLS[call], name))
            elif relative is not None:
                source.relative_names[relative[1]].add(name)
            if len(arguments) >= 3 and "/" not in name:
                fallback = default_text(arguments[2])
                if fallback is not None:
                    source.defaults[name] = fallback
        elif call in READER_TEXT_CALLS:
            # The name of a field read as text: the member it goes to, or the name a message about it gives.
            line = line_of(match.start())
            nearby = "\n".join(lines[line:line + 8])
            name = None
            assigned = re.search(r"card\.(\w+)\s*=", lines[line])
            if assigned:
                name = assigned.group(1)
            else:
                labelled = re.search(r'fieldLabel\("(\w+)"\)', nearby) or re.search(r',\s*"(\w+)"\);', nearby)
                if labelled:
                    name = labelled.group(1)
            if name and FIELD_NAME.fullmatch(name) and call != "isBlank":
                if slot is not None:
                    source.text_names.setdefault(slot, name)
                elif relative is not None:
                    source.relative_names[relative[1]].add(name)
    for match in re.finditer(r"\bwriter\.(\w+)\(", text):
        call = match.group(1)
        if call not in WRITER_CALLS:
            continue
        arguments, end = call_arguments(text, match.end() - 1)
        if len(arguments) < 2:
            continue
        slot, relative = evaluate_slot(arguments[0], constants)
        name = member_name(arguments[2] if call == "spanText" and len(arguments) > 2 else arguments[1])
        if name is None or not FIELD_NAME.fullmatch(name):
            continue
        if slot is not None:
            source.text_names.setdefault(slot, name)
        elif relative is not None:
            source.relative_names[relative[1]].add(name)
    return source


MEMBER = re.compile(r"^\s+(?!static|using|return|//)[\w:<>,\s]+?[\s>](\w+)\s*(?:=[^;]*|\{[^;]*\})?;")


def card_members(card):
    """The members of the card struct of a card type, in the order the header declares them."""
    for path in (ROOT / "include" / "oor" / "model" / "cards").rglob(card + "Card.hpp"):
        text = path.read_text(encoding="utf-8")
        match = re.search(r"struct\s+" + card + r"Card\s*\{(.*?)\n\};", text, re.S)
        if not match:
            return []
        members = []
        for line in match.group(1).splitlines():
            found = MEMBER.match(line)
            if found and "(" not in line.split("=")[0] and FIELD_NAME.fullmatch(found.group(1)):
                members.append(found.group(1))
        return members
    return []


def table_sources():
    """The table source of each card type, by its neutral name."""
    sources = {}
    for path in CARDS_FOLDER.rglob("*Table.cpp"):
        sources[path.name[: -len("Table.cpp")]] = path
    return sources


# ---------------------------------------------------------------------------------------------------------- layouts

def split_names(name, review_kinds):
    """The fields a name of the code stands for: "materialAngle/matFrameId" is two alternatives."""
    names = [part for part in name.split("/") if part]
    kinds = []
    for part in names:
        if part in review_kinds:
            kinds.append(review_kinds[part])
        elif re.search(r"(Id|Ids|Node|node\d*|Point|Grid)$", part) or part.startswith("node"):
            kinds.append("integer")
        else:
            kinds.append("real")
    return list(zip(names, kinds))


def field_kind(accessors, written_kinds):
    if "number" in accessors:
        return "number"
    if accessors == {"integer"}:
        return "integer" if written_kinds <= {"i"} else "any"
    if accessors == {"real"}:
        return "real" if written_kinds <= {"r", "i"} else "any"
    if accessors:
        return "any"
    if not written_kinds or written_kinds == {"t"}:
        return "text"
    return "any"


class CardFacts:
    """Everything known of one card type's slots."""

    def __init__(self, card, trace, source, review_kinds):
        self.card = card
        self.review_kinds = review_kinds
        self.names = defaultdict(set)  # slot -> {(accessor, name)}
        for slot, items in trace.named.items():
            self.names[slot] |= items
        for slot, items in source.fixed.items():
            self.names[slot] |= items
        self.written = defaultdict(set)  # slot -> {kind}
        for pattern in trace.writes:
            for slot, kind in pattern.items():
                self.written[slot].add(kind)
        self.writes = trace.writes
        self.text_names = source.text_names
        self.relative_names = source.relative_names
        self.defaults = source.defaults
        self.members = card_members(card)
        self.known_fields = set()
        self.variable = source.variable
        # A slot the reader only looks at (it checks that it is blank), that no writer fills and no name names, is
        # padding of the old layout.
        named_or_written = set(self.names) | set(self.written) | set(source.text_names)
        self.looked = set()
        # A text spread over slots first..last (FieldReader::spanText) is one field at its first slot.
        self.spans = {first: last - first + 1 for first, last in trace.spans}
        inner = {slot for first, last in trace.spans for slot in range(first + 1, last + 1)}
        for first in self.spans:
            self.looked.add(first)
        self.slots = sorted((named_or_written | self.looked) - inner)
        for slot in inner:
            self.written.pop(slot, None)

    def plain_names(self, slot):
        """The (name, accessor) pairs of a slot, the names with '/' split."""
        result = []
        for accessor, name in sorted(self.names.get(slot, ())):
            for part, kind in split_names(name, self.review_kinds):
                result.append((part, accessor if "/" not in name else kind))
        return result

    def name_slots(self):
        """name -> the slots it is read from."""
        slots = defaultdict(set)
        for slot in self.names:
            for name, _ in self.plain_names(slot):
                slots[name].add(slot)
        return slots


def unnamed(slot):
    return "field" + str(slot)


def fixed_fields(facts, slots):
    """The fixed fields of `slots`: one per slot, two for alternatives."""
    fields = []
    for slot in slots:
        written = facts.written.get(slot, set())
        pairs = facts.plain_names(slot)
        distinct = []
        for name, accessor in pairs:
            if name not in [item[0] for item in distinct]:
                distinct.append((name, accessor))
        if len(distinct) >= 2:
            kinds = {accessor for _, accessor in distinct}
            if len(distinct) == 2 and kinds in ({"integer", "real"}, {"integer", "text"}, {"real", "text"}):
                for name, accessor in distinct:
                    fields.append({"name": name, "slot": slot, "kind": accessor})
                continue
            # one place, several names in different branches of the reader: the first name
            accessors = {accessor for _, accessor in distinct}
            fields.append({"name": distinct[0][0], "slot": slot, "kind": field_kind(accessors, written)})
            continue
        if distinct:
            name, accessor = distinct[0]
            fields.append({"name": name, "slot": slot, "kind": field_kind({accessor}, written)})
            continue
        name = facts.text_names.get(slot) or unnamed(slot)
        fields.append({"name": name, "slot": slot, "kind": field_kind(set(), written)})
    # A name given to two places: the second one takes the name of its member, or a name of its own.
    by_name = defaultdict(list)
    for field in fields:
        by_name[field["name"]].append(field)
    for name, holders in by_name.items():
        if len({field["slot"] for field in holders}) < 2:
            continue
        keeper = next((field for field in holders if facts.text_names.get(field["slot"]) == name), holders[0])
        for field in holders:
            if field is not keeper:
                candidate = facts.text_names.get(field["slot"])
                taken = {other["name"] for other in fields}
                field["name"] = candidate if candidate and candidate not in taken else unnamed(field["slot"])
    name_gaps_from_members(facts, fields)
    for field in fields:
        if facts.spans.get(field["slot"], 1) > 1:
            field["span"] = facts.spans[field["slot"]]
    for field in fields:
        if field["name"] in facts.defaults:
            field["default"] = facts.defaults[field["name"]]
    return fields


def name_gaps_from_members(facts, fields):
    """Names the fields still without one from the members of the card struct: the members declared between the
    members of the named fields around a gap, not named yet, when there are as many of them as unnamed slots."""
    members = facts.members
    if not members:
        return
    used = {field["name"] for field in fields}
    index = 0
    while index < len(fields):
        if not re.fullmatch(r"field\d+", fields[index]["name"]):
            index += 1
            continue
        end = index
        while end < len(fields) and re.fullmatch(r"field\d+", fields[end]["name"]):
            end += 1
        before = next((members.index(fields[k]["name"]) for k in range(index - 1, -1, -1)
                       if fields[k]["name"] in members), -1)
        after = next((members.index(fields[k]["name"]) for k in range(end, len(fields))
                      if fields[k]["name"] in members), len(members))
        candidates = [member for member in members[before + 1:after] if member not in used]
        if facts.known_fields:
            known = [member for member in candidates if member in facts.known_fields]
            if len(known) == end - index:
                candidates = known
        if len(candidates) == end - index:
            for field, member in zip(fields[index:end], candidates):
                field["name"] = member
                used.add(member)
        index = end


def find_group(facts, strip_numbers=False):
    """(first slot, period) of the repeated group the names show, or None. With `strip_numbers`, the names are taken
    without the number at their end (value1, value2: value)."""
    slots_of = facts.name_slots()
    if strip_numbers:
        stripped = defaultdict(set)
        for name, slots in slots_of.items():
            stripped[re.sub(r"\d+$", "", name)] |= slots
        slots_of = stripped
    repeated = {name: slots for name, slots in slots_of.items() if len(slots) >= 2}
    if not repeated:
        return None
    differences = []
    for slots in repeated.values():
        ordered = sorted(slots)
        differences += [b - a for a, b in zip(ordered, ordered[1:])]
    period = 0
    for difference in differences:
        period = math.gcd(period, difference)
    first = min(min(slots) for slots in repeated.values())
    for slots in repeated.values():
        if len({(slot - first) % period for slot in slots}) != 1:
            return None
    # The group starts at the start of its period: a slot before `first` that is used and fits the period belongs to it
    # when no fixed name is there.
    return first, period


def group_layout(facts, first, period):
    offsets = {}
    for slot in facts.slots:
        if slot < first:
            continue
        offset = (slot - first) % period
        for name, accessor in facts.plain_names(slot):
            offsets.setdefault(offset, {}).setdefault(name, set()).add(accessor)
        offsets.setdefault(offset, {})
    fields = []
    for offset in sorted(offsets):
        names = offsets[offset]
        written = set()
        for slot in facts.slots:
            if slot >= first and (slot - first) % period == offset:
                written |= facts.written.get(slot, set())
        if names:
            # The name without a number of the repetition, when the reader gives the first one a number (scaleFactor1).
            name = sorted(names, key=lambda item: (len(item), item))[0]
            accessors = set().union(*names.values())
        else:
            candidates = sorted(facts.relative_names.get(offset, ()))
            name = candidates[0] if candidates else "value" + str(offset + 1)
            accessors = set()
        field = {"name": name, "offset": offset, "kind": field_kind(accessors, written)}
        span = max((facts.spans.get(slot, 1) for slot in facts.slots
                    if slot >= first and (slot - first) % period == offset), default=1)
        if span > 1:
            field["span"] = min(span, period - offset)
        fields.append(field)
    return {"name": "repetitions", "firstSlot": first, "period": period, "fields": fields}


def covers(layout, slot):
    group = layout.get("group")
    if group and slot >= group["firstSlot"]:
        return (slot - group["firstSlot"]) % group["period"] in {field["offset"] for field in group["fields"]}
    return any(field["slot"] == slot for field in layout["fields"])


def raw_tail(facts, first, note):
    layout = {"fields": fixed_fields(facts, [slot for slot in facts.slots if slot < first]),
              "group": {"name": "values", "firstSlot": first, "period": 1,
                        "fields": [{"name": "value", "offset": 0, "kind": "any"}]},
              "note": note}
    return layout


def holds_together(layout):
    """The checks of CardLayouts::fromJson that the generator can break: spans that cover another field, a group field
    whose span leaves its period or covers another offset, fixed fields in the slots of the group."""
    fields = layout["fields"]
    for field in fields:
        for other in fields:
            if other is not field and other["slot"] != field["slot"] and \
                    field["slot"] < other["slot"] < field["slot"] + field.get("span", 1):
                return False
    group = layout.get("group")
    if group:
        if any(field["slot"] + field.get("span", 1) - 1 >= group["firstSlot"] for field in fields):
            return False
        taken = set()
        for field in group["fields"]:
            covered = set(range(field["offset"], field["offset"] + field.get("span", 1)))
            if max(covered) >= group["period"] or covered & taken:
                return False
            taken |= covered
    return True


def unique_names(layout):
    names = [field["name"] for field in layout["fields"]]
    if layout.get("group"):
        names += [field["name"] for field in layout["group"]["fields"]]
    return len(names) == len(set(names))


def make_layout(facts):
    # Only a reader that goes through the fields up to the last one used reads a repeated group; a name that comes
    # back in a card of fixed fields is a name the reader gives twice (the second place gets its own name).
    variable = facts.variable or any(len(slots) >= 3 for slots in facts.name_slots().values())
    group = find_group(facts) if variable else None
    if group is None and variable:
        group = find_group(facts, strip_numbers=True)
    if group is None and not variable:
        layout = {"fields": fixed_fields(facts, facts.slots)}
    elif group is None:
        # A reader that looks at the last field used, with no name that comes back: when every slot a writer filled
        # has a name, the reader only checks that nothing follows the fixed fields; otherwise it reads a list, and the
        # slots after the last fixed name stay as they are.
        fixed = [slot for slot in facts.slots if facts.names.get(slot) and len(facts.plain_names(slot)) >= 1]
        known = set(fixed) | set(facts.text_names)
        if facts.written and known and max(facts.written) <= max(known):
            layout = {"fields": fixed_fields(facts, facts.slots)}
        else:
            first = (max(fixed) + 1) if fixed else 1
            layout = raw_tail(facts, first, "a list read up to the last field used: its slots are kept as they are")
    else:
        first, period = group
        layout = {"fields": fixed_fields(facts, [slot for slot in facts.slots if slot < first]),
                  "group": group_layout(facts, first, period)}
    # Every slot a writer filled must have a place; otherwise the slots from the first one without a place are kept.
    missing = sorted(slot for slot in facts.written if not covers(layout, slot))
    if missing or not unique_names(layout) or not holds_together(layout):
        first = missing[0] if missing else (layout["group"]["firstSlot"] if layout.get("group") else 1)
        if layout.get("group"):
            first = min(first, layout["group"]["firstSlot"])
        layout = raw_tail(facts, first, "slots the generator could not describe: kept in the slot order of the old "
                                        "layout from slot " + str(first))
        if not unique_names(layout) or not holds_together(layout):
            layout = raw_tail(facts, 1, "slots the generator could not describe: kept in the slot order of the old "
                                        "layout")
    return layout


def is_flat(value):
    if isinstance(value, dict):
        return all(not isinstance(item, (dict, list)) for item in value.values())
    if isinstance(value, list):
        return all(not isinstance(item, (dict, list)) for item in value)
    return True


def layout_part_text(value, indent):
    """A group or the sections of a layout: the scalar members inline, every field on a line of its own."""
    if is_flat(value):
        return json.dumps(value, separators=(", ", ": "))
    if isinstance(value, list):
        if not value:
            return "[]"
        items = ",\n".join(" " * (indent + 2) + layout_part_text(item, indent + 2) for item in value)
        return "[\n" + items + "\n" + " " * indent + "]"
    members = [f"{json.dumps(key)}: {layout_part_text(item, indent)}" for key, item in value.items()]
    return "{" + ", ".join(members) + "}"


def layout_members(layout, indent):
    """The members of a layout (or of a form), one field on a line."""
    def one_line(value):
        return json.dumps(value, separators=(", ", ": "))

    pad = " " * indent
    members = []
    for key in ("name", "when"):
        if key in layout:
            members.append(f'{pad}"{key}": {one_line(layout[key])}')
    if "fields" in layout or "forms" not in layout:
        fields = ",\n".join(pad + " " + one_line(field) for field in layout.get("fields", []))
        members.append(f'{pad}"fields": [' + ("\n" + fields + "\n" + pad + "]" if fields else "]"))
    for part in ("group", "nextGroups", "sections"):
        if layout.get(part):
            members.append(f'{pad}"{part}": ' + layout_part_text(layout[part], indent))
    if layout.get("forms"):
        forms = [pad + " {\n" + ",\n".join(layout_members(form, indent + 2)) + "\n" + pad + " }"
                 for form in layout["forms"]]
        members.append(f'{pad}"forms": [\n' + ",\n".join(forms) + "\n" + pad + "]")
    if layout.get("note"):
        members.append(f'{pad}"note": {json.dumps(layout["note"])}')
    return members


def catalogue_text(catalogue):
    """The catalogue as JSON text that reads well and diffs well: one line per field."""
    lines = ["{", f' "description": {json.dumps(catalogue["description"])},',
             f' "formatVersion": {catalogue["formatVersion"]},', ' "cards": {']
    names = sorted(catalogue["cards"])
    for number, name in enumerate(names):
        lines.append(f"  {json.dumps(name)}: {{")
        lines.append(",\n".join(layout_members(catalogue["cards"][name], 3)))
        lines.append("  }" + ("," if number + 1 < len(names) else ""))
    lines += [" }", "}"]
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------------------------------------- checks

KINDS = {"integer", "real", "number", "text", "any"}
NUMERIC = {"integer", "real", "number"}
VALUE_KINDS = {"blank", "integer", "real", "text"}


def last_offset(group):
    return max(field["offset"] + field.get("span", 1) - 1 for field in group["fields"])


def line_length(group):
    return (group["perLine"] - 1) * group["period"] + last_offset(group) + 1


def kinds_differ(a, b):
    """Two kinds a value tells apart (kindsDiffer in CardLayouts.cpp)."""
    kinds = {a, b}
    return a != b and "any" not in kinds and not ("number" in kinds and "text" not in kinds)


def is_word(text):
    return isinstance(text, str) and bool(text) and not NUMBER_LITERAL.match(text)


def condition_field_exists(layout, name):
    if any(field["name"] == name for field in layout.get("fields", [])):
        return True
    group = layout.get("group")
    if group and any(field["name"] == name for field in group["fields"]):
        return True
    sections = layout.get("sections")
    return bool(sections and not group and sections.get("name") == name)


def check_card(card, layout):
    """The problems CardLayouts::fromJson would find in the layout of a card, its forms included."""
    if "forms" not in layout:
        return check_layout(card, layout)
    problems = []
    if layout.get("fields") or layout.get("group") or layout.get("nextGroups") or layout.get("sections"):
        problems.append(f"{card}: a card with forms has no field, group or sections of its own")
    forms = layout["forms"]
    if len(forms) < 2:
        problems.append(f"{card}: a card with forms has two of them or more")
    names = set()
    for index, form in enumerate(forms):
        at = f"{card}, form {form.get('name')}"
        if not form.get("name") or form["name"] in names:
            problems.append(f"{at}: every form has a name of its own")
        names.add(form.get("name"))
        if index > 0 and "when" not in forms[index - 1]:
            problems.append(f"{at}: only the last form has no condition")
        problems += check_layout(at, form)
        when = form.get("when")
        if when is None:
            continue
        on_value = bool(when.get("values")) or bool(when.get("kinds"))
        on_field = bool(when.get("field")) and on_value
        on_slot = ("blankSlot" in when) + ("filledSlot" in when)
        if on_field == (on_slot > 0) or on_slot > 1 or (on_slot and (when.get("field") or on_value)):
            problems.append(f"{at}: a condition gives a field with values or kinds, or one blankSlot or filledSlot")
        if when.get("field") and not condition_field_exists(form, when["field"]):
            problems.append(f"{at}: the form has no field {when['field']}")
        for kind in when.get("kinds", []):
            if kind not in VALUE_KINDS:
                problems.append(f"{at}: unknown kind of value {kind}")
    return problems


def check_layout(card, layout):
    """The problems CardLayouts::fromJson would find in a layout (the same rules), as a list of texts."""
    problems = []
    names = set()

    def add_name(name, at, taken=names):
        if not name:
            problems.append(f"{at}: a field has no name")
        elif name in taken:
            problems.append(f"{at}: the field name {name} is used twice")
        taken.add(name)

    def check_field(field, at, place):
        if field.get("kind") not in KINDS:
            problems.append(f"{at}: unknown kind {field.get('kind')}")
        if not isinstance(field.get(place), int):
            problems.append(f"{at}: no {place}")
        if field.get("span", 1) < 1:
            problems.append(f"{at}: the span is below 1")

    fields = layout.get("fields")
    if not isinstance(fields, list):
        return [f"{card}: no fields"]
    # A span of 0 runs to the end of the card: the last fixed field, with no group or sections after it.
    for index, field in enumerate(fields):
        if field.get("span") == 0:
            if index + 1 != len(fields) or layout.get("group") or layout.get("sections"):
                problems.append(f"{card}: the field {field.get('name')} runs to the end of the card (span 0): it is "
                                f"the last field, with no group or sections")
    fields = [dict(field, span=SPAN_TO_END) if field.get("span") == 0 else field for field in fields]
    for field in fields:
        at = f"{card}, field {field.get('name')}"
        add_name(field.get("name"), at)
        check_field(field, at, "slot")
        if isinstance(field.get("slot"), int) and field["slot"] < 1:
            problems.append(f"{at}: the slot is below 1")
    if problems:
        return problems
    for first, a in enumerate(fields):
        for second in range(first + 1, len(fields)):
            b = fields[second]
            span_a, span_b = a.get("span", 1), b.get("span", 1)
            overlaps = a["slot"] < b["slot"] + span_b and b["slot"] < a["slot"] + span_a
            if overlaps and a["slot"] != b["slot"]:
                problems.append(f"{card}: the field {b['name']} is in the span of {a['name']}")
            if a["slot"] != b["slot"]:
                continue
            if second != first + 1:
                problems.append(f"{card}: the alternatives {a['name']} and {b['name']} are not next to each other")
            if not kinds_differ(a["kind"], b["kind"]):
                problems.append(f"{card}: the fields {a['name']} and {b['name']} share a slot but their kinds do "
                                f"not tell them apart")

    def check_fields(items, at, period, taken):
        offsets = set()
        for field in items:
            field_at = f"{at}, field {field.get('name')}"
            add_name(field.get("name"), field_at, taken)
            check_field(field, field_at, "offset")
            offset, span = field.get("offset", 0), field.get("span", 1)
            if offset < 0 or offset + span > period:
                problems.append(f"{field_at}: the offset is not inside the period")
            for slot in range(offset, offset + span):
                if slot in offsets:
                    problems.append(f"{field_at}: two fields have the offset {slot}")
                offsets.add(slot)

    def check_group(group, at, place, taken):
        """place: "first", "next" or "list" (the list of a section)."""
        if not group.get("name"):
            problems.append(f"{at}: the group has no name")
        start = group.get("offset" if place == "list" else "firstSlot")
        if place == "next":
            if "firstSlot" in group:
                problems.append(f"{at}: a group after the first one has no firstSlot")
            start = 1
        period = group.get("period")
        if not isinstance(start, int) or not isinstance(period, int) or period < 1:
            problems.append(f"{at}: firstSlot (offset) and period are integers, the period 1 or more")
            return False
        if not group.get("fields"):
            problems.append(f"{at}: the group has no field")
            return False
        if "count" in group:
            if place == "list":
                problems.append(f"{at}: the list of a section has no count")
            elif isinstance(group["count"], int) and group["count"] < 1:
                problems.append(f"{at}: a fixed count is 1 or more")
        if "marker" in group and (place != "next" or not is_word(group["marker"])):
            problems.append(f"{at}: only a group after the first one has a marker, a word")
        check_fields(group["fields"], at, period, taken)
        nested = group.get("list")
        if nested:
            if place == "list":
                problems.append(f"{at}: the list of a section has no nested list")
            elif not nested.get("fields"):
                problems.append(f"{at}: the nested list has no field")
            else:
                list_at = f"{at}, list {nested.get('name')}"
                check_fields(nested["fields"], list_at, len(nested["fields"]), taken)
                if any(field.get("span", 1) != 1 for field in nested["fields"]):
                    problems.append(f"{list_at}: the fields of a nested list have no span")
                if group["fields"][0].get("offset") != 0 or nested["fields"][0].get("offset") != 0 or \
                        not kinds_differ(group["fields"][0]["kind"], nested["fields"][0]["kind"]):
                    problems.append(f"{list_at}: the first field of the group and the first field of its list, each "
                                    f"at offset 0, are of kinds a value tells apart")
                if group.get("perLine", 0) > 0:
                    problems.append(f"{at}: a group with a nested list gives no perLine")
        return True

    group = layout.get("group")
    earlier = set(names)
    groups = []
    if group:
        at = f"{card}, group {group.get('name')}"
        if check_group(group, at, "first", names):
            if group["firstSlot"] < 1:
                problems.append(f"{at}: firstSlot is 1 or more")
            if group.get("perLine", 0) > 0 and (group["firstSlot"] - 1) % LINE_SLOTS + line_length(group) > LINE_SLOTS:
                problems.append(f"{at}: perLine repetitions do not fit in a line")
            for field in fields:
                if field["slot"] + field.get("span", 1) - 1 >= group["firstSlot"]:
                    problems.append(f"{card}: the field {field['name']} is in the slots of the group")
        groups.append(group)
    for next_group in layout.get("nextGroups", []):
        at = f"{card}, group {next_group.get('name')}"
        if not group:
            problems.append(f"{card}: nextGroups follow a group")
            break
        if check_group(next_group, at, "next", names):
            if next_group.get("perLine", 0) > 0 and line_length(next_group) > LINE_SLOTS:
                problems.append(f"{at}: perLine repetitions do not fit in a line")
        groups.append(next_group)
    for item in groups:
        count = item.get("count")
        if isinstance(count, str) and count not in earlier:
            problems.append(f"{card}, group {item['name']}: the count field {count} is not a field given before it")
        earlier |= {field["name"] for field in item["fields"]}
    for before, after in zip(groups, groups[1:]):
        if "count" not in before and not after.get("marker"):
            problems.append(f"{card}: the group {before['name']} is followed by {after['name']}: it gives a count, or "
                            f"the next group a marker")
        if after.get("marker") and (before.get("perLine", 0) > 0 or before.get("list")):
            problems.append(f"{card}: the group {before['name']} before a marker has no perLine and no nested list")
    sections = layout.get("sections")
    if sections and layout.get("nextGroups"):
        problems.append(f"{card}: a card with several groups has no sections")
    if sections:
        problems += check_sections(card, sections, fields, names, group, check_group)
    return problems


def check_sections(card, sections, fields, names, group, check_group):
    """The problems of the sections of a layout (check_layout)."""
    problems = []
    at = f"{card}, sections {sections.get('name')}"
    packed = sections.get("packed", False)
    first = sections.get("firstSlot", 0)
    if group and first:
        problems.append(f"{at}: sections after a group start on the line after it: no firstSlot")
    if not group and packed and first < 1:
        problems.append(f"{at}: firstSlot is 1 or more")
    if not group and not packed and (first < 1 or (first - 1) % LINE_SLOTS):
        problems.append(f"{at}: firstSlot is the first slot of a line (1, 9, 17, ...)")
    if packed and "keyOffset" in sections:
        problems.append(f"{at}: packed sections have their key word first: no keyOffset")
    key_offset = 0 if packed else sections.get("keyOffset")
    if not isinstance(key_offset, int) or not 0 <= key_offset < LINE_SLOTS:
        problems.append(f"{at}: keyOffset is from 0 to 7")
        return problems
    keys = set()
    for kind in sections.get("kinds", []):
        kind_at = f"{at}, key {kind.get('key')!r}"
        key = kind.get("key")
        if not isinstance(key, str):
            problems.append(f"{kind_at}: no key")
            continue
        if key.upper() in keys:
            problems.append(f"{kind_at}: two kinds have this key")
        keys.add(key.upper())
        own_offset = kind.get("keyOffset", key_offset)
        if "keyOffset" in kind and (not 0 <= own_offset < LINE_SLOTS or packed or not key):
            problems.append(f"{kind_at}: the keyOffset of a kind is from 0 to 7, for a kind with a key word of "
                            f"sections that are not packed")
        taken_names = set(names)
        taken = set()
        for field in kind.get("fields", []):
            field_at = f"{kind_at}, field {field.get('name')}"
            if not field.get("name") or field["name"] in taken_names:
                problems.append(f"{field_at}: the field name is missing or used twice")
            taken_names.add(field.get("name"))
            if field.get("kind") not in KINDS:
                problems.append(f"{field_at}: unknown kind {field.get('kind')}")
            offset, span = field.get("offset", 0), field.get("span", 1)
            if packed:
                for place in range(offset, offset + span):
                    if place < 1 or place in taken:
                        problems.append(f"{field_at}: the field is in the place of the key word or of a field")
                    taken.add(place)
                continue
            if offset < 0 or offset % LINE_SLOTS + span > LINE_SLOTS:
                problems.append(f"{field_at}: the field is not inside a line")
            for place in range(offset, offset + span):
                number_allowed = place >= LINE_SLOTS or not key
                if place % LINE_SLOTS == own_offset and not (number_allowed and field.get("kind") in NUMERIC):
                    problems.append(f"{field_at}: the field is in the place of the key word")
                if place in taken:
                    problems.append(f"{field_at}: two fields have the offset {place}")
                taken.add(place)
        if packed and not key:
            problems.append(f"{kind_at}: a kind of packed sections has a key word")
        lst = kind.get("list")
        if not lst:
            continue
        list_at = f"{kind_at}, list {lst.get('name')}"
        if not check_group(lst, list_at, "list", taken_names):
            continue
        per_line = lst.get("perLine", 0)
        if packed:
            if per_line:
                problems.append(f"{list_at}: the list of a packed section gives no perLine")
            if lst["offset"] < 1:
                problems.append(f"{list_at}: the list comes after the key word")
        elif per_line == 0:
            if any(field.get("kind") not in NUMERIC for field in lst["fields"]):
                problems.append(f"{list_at}: a list with no perLine runs over the key places of its lines: it holds "
                                f"numbers")
            if lst["offset"] <= own_offset:
                problems.append(f"{list_at}: the list is in the place of the key word")
        else:
            in_line = lst["offset"] % LINE_SLOTS
            end = in_line + line_length(lst)
            if lst["offset"] < 0 or end > LINE_SLOTS:
                problems.append(f"{list_at}: the repetitions of a line do not fit in it")
            if in_line <= own_offset < end:
                problems.append(f"{list_at}: the list is in the place of the key word")
        if any(place >= lst["offset"] for place in taken):
            problems.append(f"{list_at}: the fields of a kind with a list come before the list")
    if not sections.get("kinds"):
        problems.append(f"{at}: the sections have no kind")
    if first and not group:
        for field in fields:
            if field["slot"] < first:
                continue
            if packed:
                problems.append(f"{card}: the field {field['name']} is in the slots of the sections")
                continue
            offset = field["slot"] - first
            taken = offset >= LINE_SLOTS or field.get("span", 1) > 1 or offset == key_offset
            for kind in sections.get("kinds", []):
                taken = taken or offset == kind.get("keyOffset", key_offset)
                for other in kind.get("fields", []):
                    in_line = other["offset"] % LINE_SLOTS
                    taken = taken or in_line <= offset < in_line + other.get("span", 1)
                lst = kind.get("list")
                if lst:
                    if lst.get("perLine", 0) == 0:
                        taken = True
                    else:
                        in_line = lst["offset"] % LINE_SLOTS
                        taken = taken or in_line <= offset < in_line + line_length(lst)
            if taken:
                problems.append(f"{card}: the field {field['name']} is in the slots of the sections")
    return problems


# ---------------------------------------------------------------------------------------------------------- review

def read_review():
    """The kinds of alternatives (review.json) and the reviewed cards (review/*.json), by card name."""
    review = json.loads(REVIEW.read_text(encoding="utf-8")) if REVIEW.exists() else {}
    cards = {}
    for path in sorted(REVIEW_FOLDER.glob("*.json")):
        for card, entry in json.loads(path.read_text(encoding="utf-8")).get("cards", {}).items():
            if card in cards:
                raise SystemExit(f"{path.name}: the card {card} is reviewed twice")
            cards[card] = entry
    return review.get("kinds", {}), cards


def apply_names(card, layout, names):
    """Gives the fields (and the group) of a generated layout the names of the review."""
    unused = dict(names)
    for field in layout["fields"]:
        if field["name"] in unused:
            field["name"] = unused.pop(field["name"])
    group = layout.get("group")
    if group:
        if group["name"] in unused:
            group["name"] = unused.pop(group["name"])
        for field in group["fields"]:
            if field["name"] in unused:
                field["name"] = unused.pop(field["name"])
    if unused:
        raise SystemExit(f"review of {card}: no field named {', '.join(sorted(unused))} in the generated layout")
    return layout


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--traces", required=True, help="folder of the trace files")
    parser.add_argument("--output", default=str(ROOT / "formats" / "oos-layouts.json"))
    arguments = parser.parse_args()

    review_kinds, reviewed = read_review()
    traces = read_traces(arguments.traces)
    sources = table_sources()

    cards = {}
    summary = defaultdict(int)
    problems = []
    for card in sorted(set(reviewed) | set(traces) | set(sources)):
        entry = reviewed.get(card, {})
        if "fields" in entry:
            layout = entry
            summary["reviewed: whole layout"] += 1
        else:
            facts = CardFacts(card, traces.get(card, Trace()), read_source(sources.get(card)), review_kinds)
            facts.known_fields = set(entry.get("names", {}).values())
            layout = make_layout(facts)
            if card not in traces:
                summary["never traced"] += 1
            if "names" in entry:
                layout = apply_names(card, layout, entry["names"])
                summary["reviewed: names"] += 1
            elif card in reviewed:
                summary["reviewed: as generated"] += 1
            else:
                summary["not reviewed"] += 1
        if layout.get("note"):
            summary["kept slots"] += 1
        if layout.get("group"):
            summary["repeated group"] += 1
        if layout.get("sections"):
            summary["sections"] += 1
        if layout.get("forms"):
            summary["forms"] += 1
        fields = layout.get("fields", [])
        if any(re.fullmatch(r"field[0-9]+", field["name"]) for field in fields):
            summary["unnamed fields"] += 1
        if len({field["slot"] for field in fields}) != len(fields):
            summary["alternatives"] += 1
        problems += check_card(card, layout)
        cards[card] = layout
    unknown = sorted(set(reviewed) - set(cards))
    if unknown:
        problems.append("reviewed cards without a source layout: " + ", ".join(unknown))
    if problems:
        print("\n".join(problems), file=sys.stderr)
        return 1

    catalogue = {
        "description": "The neutral card layout catalogue of the O_o model format (.oos). Fields are ordered "
                       "record values; slots connect them to card readers and writers. Groups describe repeated "
                       "values, sections contain a neutral key and their fields, and forms select another layout "
                       "from preceding values. Generated from source slot traces and reviewed model-format layouts. "
                       "docs/model-format-cards.md is generated from this catalogue.",
        "formatVersion": FORMAT_VERSION,
        "cards": cards,
    }
    Path(arguments.output).write_text(catalogue_text(catalogue), encoding="utf-8", newline="\n")
    print(f"{len(cards)} card layouts written to {arguments.output}")
    for key in sorted(summary):
        print(f"  {key}: {summary[key]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
