# The O_o text model format

O_o_Reader reads and writes `.oos` model files. O_o_Solver consumes the reader's typed table packages.

The same model can be saved in `.oorh5` as native typed HDF5 columns, with optional result tables. See
[the archive format](archive-format.md#native-model) for its binary layout and compatibility rules. `oor.readModel`
recognizes both formats by content; this page specifies the `.oos` text syntax and compact field order.

## File syntax

- UTF-8 text, extension `.oos`, one record per line. A record never spans two lines.
- Values are separated by `;`. Spaces around an unquoted value are ignored.
- A value containing `;`, `"` or leading or trailing spaces is quoted with `"`. A quote inside it is doubled.
  A first value beginning with `#` or `[` is also quoted so it is read as record data.
- `#` at the start of a line, after optional spaces, makes the line a comment. Comments belong to the following record
  and are retained when the model is written.
- Empty lines are ignored. Empty values take their field's default; trailing empty values are omitted on output.

## Values

An integer has an optional sign and digits (`12`, `-3`). A real has a decimal point or exponent (`1.0`, `2.`,
`-1.5e-3`, `7E2`), which distinguishes it from an integer where the two are alternatives. A name is exact and
case-sensitive: `ShellProp`, `Through`, `Yes`, `All`. The card reference lists the accepted words.

Names and keyword values use the same neutral names as the code and API. Literal labels, identifiers, file paths,
equations and other free text retain their contents. User labels are not interpreted as keyword values.

## Sections

A section begins with a bracketed line. A model has up to three main sections:

```text
[executive]
SolutionSequence;LinearStatics
[case]
TitleText;Bracket under load
DisplacementOutput;All;PrintListing;PlotOnly
SubcaseStart;1
StaticLoadSelection;10
SubcaseStart;2
StaticLoadSelection;20
[model]
Node;1;;0.0;0.0;0.0
ShellQuad4;12;101;1;2;3;4
ShellProp;101;1;0.002
```

The executive section describes the analysis and its external files. A statement is its neutral name followed by its
values. A parameter is `name`, `name=value` or `name(options)=value`. `FileAssignment` associates a path with a file
unit; paths and external labels are literal text.

The case section describes selections and output requests. A command is its name, its value, then its options, each
a name or `name=value`. Lines before the first `SubcaseStart;n` apply globally. A subcase contains the records after
its header up to the next one. `SubcaseParameter;name;value` sets a parameter. A set is, for example,
`SetDefinition;3;1;2;5;Through;10`; `Through`, `By`, `Except` and `All` describe its membership.

The model section contains one card per record. Records before the first section line are also model cards.

## Cards

Each card record starts with its neutral card name, followed by its fields in compact order. For example:

```text
ShellQuad4;12;101;1;2;3;4;30.;;0.01
```

The [card reference](model-format-cards.md) is the authoritative field order. It describes fixed fields, alternatives,
repeated groups, section keys and forms. An alternative has a field of its own: the example above specifies a material
angle, leaves the alternative material frame blank, then gives an offset.

Repeated groups follow the fixed fields, one group after another. When the boundary of a variable-length group cannot
be inferred, the record gives its item count before its items. A section consists of its key, fixed fields and,
when present, the count and items of its list. Forms select another field order according to a preceding value.
A text field remains one value regardless of its length. Quoting preserves surrounding spaces. A record value cannot
contain CR or LF; writing such a value raises an error before output files are replaced.

A card name the library does not know is retained together with its values. A known card that fails validation is
retained as text in tolerant mode; strict mode stops at the first error. A malformed record is diagnosed with its
source file, line, card and field.

## Includes and parts

`Include;path` inserts another model file at that place. Its path is relative to the including file, or absolute.
Includes work in every section. The writer can flatten the model or preserve the include structure.

A part opens with `[model;SuperelementPart;5]`, or another neutral part kind. The part has its own cards and identifiers.
`[model]` returns to the main model. Part options can specify `massId=300` or `append` after the number.

## Design targets

A design relation names the target card type and a neutral field descriptor. A descriptor is a field or column name,
with a one-based item or station when needed: `thickness`, `plyThickness(3)`, `area(EndA)`, `area(EndB)` or `area(3)`.
Each card lists the fields that design relations can drive. Unknown descriptors produce a refusal rather than an
inferred target.

## APIs and diagnostics

C++ uses `ModelFileReader` and `ModelFileWriter`. JavaScript uses `readModel`, `writeModel` and `toModelText`.
The table packages between the reader and solver expose neutral card, column and word names.
Diagnostics identify the same names as the file, for example `ShellQuad4 12, field offset`.
