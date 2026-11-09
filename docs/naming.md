# Names

O_o_Reader, O_o_Solver and `.oos` model files use the same neutral names. Names are exact and case-sensitive.

| What | Convention | Example |
| --- | --- | --- |
| Card type, result element type and C++ type | UpperCamelCase | `ShellQuad4`, `CompProp` |
| C++ function, member field, stored column and query | lowerCamelCase | `propId`, `thickness`, `nodesTable` |
| Word a field accepts | UpperCamelCase | `Yes`, `All`, `Through`, `Mass` |
| Card source directory | lowerCamelCase | `src/model/cards/elements/shellQuad4/` |
| Constant | UPPER_CASE | `NO_THICKNESS` |
| Class member | `m_` followed by lowerCamelCase | `m_tables` |

A message identifies a card and a field by the same names the file and API use: `ShellQuad4 12, field offset`.
The reader has no keyword dictionary or translated labels. O_o_Converter owns translation to and from external
input formats.

See [element names](element-naming.md) for the shared physical-family vocabulary, implementation roles and compatibility.

## Model fields and words

`formats/oos-layouts.json` gives each card's fields, alternatives, repeated groups, sections and forms.
`docs/model-format-cards.md` is its generated reference. The compact order described there is the record order;
internal field placement is an implementation detail.

`formats/value-fields.json` distinguishes keyword values from literal labels, identifiers and text. Literal values
retain their contents: a label spelled `YES` remains that label. A keyword field uses its accepted neutral word.
Matrix identifiers are normally user names; the reserved aerodynamic names are `DownwashMatrix`, `AeroPressureMatrix`,
`AeroWeightingMatrix` and `AeroWeightingFactor`.

Design target descriptors use the corresponding field or column name, with a one-based item or station when needed:
`thickness`, `plyThickness(3)`, `area(EndA)`, `area(EndB)` or `area(3)`. The reader does not infer a field from the
position of a value in another input format.

The executive, case and parameter catalogues use the names written by model records: `SolutionSequence`,
`SubcaseStart`, `DisplacementOutput`, `PrintListing`, `WeightToMass` and `OffsetDefinition`. The JavaScript API, query
names, query columns and query values expose those names directly.

## Data contract

The table-package contract is version 75. Stored columns and computed query columns use neutral field names; word
values use neutral names too. A schema or value change that makes a consumer incompatible raises
`DATA_CONTRACT_VERSION` once for the change. The solver verifies that version when it imports tables.

To add a card or field, update its typed table, registry entry, layout, field semantics and generated reference.
Keep the code, model records, diagnostics and exported columns consistent. Run `scripts/check.sh` after the change.
