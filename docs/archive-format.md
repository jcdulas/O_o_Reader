# The HDF5 model and results archive (format version 4)

The archive is the file format this library writes for models, analysis results and custom outputs. A `.ooh5` may
contain a model alone, results alone, or both. Model data is stored as native typed columns, not as embedded `.oos` text.
It is an ordinary HDF5 file with a layout of our own: any HDF5 tool (h5py, h5dump, HDFView, h5wasm, ...) can open it, and
nothing in it follows the layout of another software vendor's files.

The conventional filename extension is `.ooh5`. Archives are recognized by their contents: changing the suffix does
not alter the HDF5 layout or its versions. The archive version remains 4 and the native model schema remains 1.

Root names and version constants are declared in
[ArchiveFormat.hpp](../include/oor/results/archive/ArchiveFormat.hpp). Result tables are written by
[ArchiveWriter.cpp](../src/results/archive/ArchiveWriter.cpp) and read by
[ArchiveResultFile.cpp](../src/results/archive/ArchiveResultFile.cpp). Native model dataset paths and the column codec
are implemented in [ArchiveModel.cpp](../src/results/archive/ArchiveModel.cpp).

## Why this layout

The archive is designed for a web viewer that opens a file once and then shows one table, one step at a time:

| Need | Choice |
| --- | --- |
| Open a large file fast | Everything needed to list the tables is in a few small datasets under `/index`: one read each, no walk over thousands of groups. |
| Open results without loading geometry | The optional `/model` section is independent and loaded only when requested. |
| Load model numbers without text parsing | Integer and real fields are contiguous typed columns, grouped by card type. |
| Read one model field | Its column descriptor gives the exact slice of a numeric buffer; no other card types need to be read. |
| Show one step of one table | The values of a table are one dataset `(steps, rows, components)` chunked by whole steps: one step is read and inflated alone. |
| Small files | Shuffle + deflate on every dataset of more than 1024 values (level 4 by default, 0 to 9), and the precision of the source is kept: single precision values stay `float32`. |
| Readable by anyone | Plain HDF5 types only (integers, floats, UTF-8 strings), plain English names, enumerations stored as their names (`"time"`, `"float32"`), never as codes. |

## Layout

```text
/                                attributes (fixed-length UTF-8 strings):
                                   format          "oor-results-archive"
                                   format_version  "4"
                                   producer        "oor_reader 0.2.0"
                                   created_at      "2026-11-05T10:00:00Z"
                                   title, subtitle, label
/model/                          optional native model, described below
/index/tables                    compound dataset, one row per result table (see below)
/index/steps                     float64 [total steps]: the steps of every table, one after the other
/index/eigenvalues               float64 [total steps]  (NaN where a table has none)
/index/eigenvalues_imag          float64 [total steps]
/index/cycles                    float64 [total steps]
/index/named                     compound dataset, one row per named result
/data/tables/000001/             one group per result table, numbered in the order written
    values                       float32 or float64, shape (steps, rows, components);
                                 complex values: shape (steps, rows, components, 2) = (real, imaginary)
    components                   strings [components]: "t1", "oxx", "vonMises", ...
    steps                        float64 [steps]: time, frequency, mode number, load factor; NaN for a static table
    eigenvalues, eigenvalues_imag, cycles   float64 [steps]   (only when the table has them)
    ids                          int32 [rows]: grid point or element of each row
    point_types                  int32 [rows]: 1 grid point, 2 scalar point, 3 extra point   (nodal tables)
    node_ids                     int32 [rows]: grid point of a row of an element, 0 at the centre
    element_ids                  int32 [rows]: element of a grid point force
    layers                       int32 [rows]: fibre (1 lower, 2 upper), ply or station of a row, from 1
    positions                    float64 [rows]: position along a beam
    text_names                   strings [n]: the text labels of the rows ("elementName", "fiber", ...)
    texts/<i>/dictionary         strings: the distinct texts of label i
    texts/<i>/codes              uint32 [rows]: the position of the text of each row in the dictionary
/data/named/000001/              one group per named result (a matrix, a grid point weight, ...)
    array_names                  strings [arrays]
    array_descriptions           strings [arrays]
    array_types                  strings [arrays]: "int32", "int64", "float32", "float64", "complex64", "complex128"
    array_shapes                 strings [arrays]: "3,4" ("" for a single number)
    arrays/<i>                   the i-th array (a complex array has a last dimension of 2)
    attributes                   strings [n, 2]: (key, value) pairs
    text_names, texts/<i>/...    as for a table
```

Only the datasets a table needs are written: a nodal table has no `layers`, a static table no `eigenvalues`.

## Native model

`/model` has fixed UTF-8 attributes `format = "oor-column-model"` and `format_version = "1"`. This schema has its own
version so its slot contract can evolve independently of the result tables. Unknown model schema versions are refused
when the model is requested. Merely listing results does not load or validate model values.

Every model array is one-dimensional. Numeric datasets have fixed HDF5 integer or IEEE float types and are converted to
native byte order by HDF5 when read. A **string vector** at path `p` consists of `p/offsets` (`uint64`, length N+1) and
`p/bytes` (`uint8`, UTF-8): string i is bytes `[offsets[i], offsets[i+1])`. Empty strings and vectors are supported.
Offsets start at zero, increase monotonically and end at the byte count. Text is compressed along with numeric data;
model datasets do not use variable-length HDF5 strings.

```text
/model/schema/card_layouts       string vector [1]: JSON catalogue of neutral fields and their slots
/model/index/
    parents                      int32 [parts]: -1 for the root, otherwise an earlier part index
    ids, mass_ids                int64 [parts]
    appends                      uint8 [parts]: 0 or 1
    kinds                        string vector [parts]: empty for the root
/model/sources/
    paths, includes              string vectors [files]: source descriptors, never opened while reading
    parents                      int32 [files]: -1 for the main file, otherwise an earlier file index
    lines                        uint32 [files]: source Include line
/model/parts/<p>/                 p is the decimal part index, starting at 0 for the root
    next_key                     uint32 [1]: next creation key (removed keys are never reused)
    card_types                   string vector [tables]: exact neutral type names, in registry order
    comments/keys                uint32 [comments]: card keys, in increasing order
    comments/text                string vector [comments]
    markers/keys                 uint32 [markers]: marker position before the next card key
    markers/kinds                uint8 [markers]: 0 Include begin, 1 Include end, 2 comments
    markers/files                uint32 [markers]: included source index for Include begin
    markers/text                 string vector [markers]
    placements/keys, files       uint32 [placements]: card key and destination source index
    documents/names              string vector [documents]: application-owned document names (optional)
    documents/text               string vector [documents]: UTF-8 document payloads (optional)
    control/executive/            structured control records (see below)
    control/case/                 structured control records
    cards/<t>/                   table t in card_types, decimal index starting at 0
        keys                     uint32 [rows]: persistent card keys, increasing, unique across the part
        ids                      int32 [rows]: entity identifiers for direct viewer lookup (0 when absent)
        field_counts             uint32 [rows]: number of slots in each row, including padding
        columns/slots            uint32 [columns]: 1-based slot number
        columns/kinds            uint8 [columns]: 1 integer, 2 real, 3 text
        columns/value_offsets    uint64 [columns]: start in the buffer selected by kind
        columns/counts           uint64 [columns]: number of values in that column
        columns/row_offsets      uint64 [columns]: sparse row slice, UINT64_MAX dense, UINT64_MAX-1 constant
        row_indices              uint32 [sparse entries]: increasing row indices within each column slice
        integers                 int64 [integer values]: concatenated integer columns
        reals                    float64 [real values]: concatenated real columns, exact binary values
        text_codes               uint32 [text values]: concatenated dictionary-coded text columns
        strings                  string vector: text dictionary of this card table
        precision/offsets        uint64 [rows+1]: source precision records, indexed by row
        precision/digits         uint8 [precision entries]: source significant digits, in slot order
        default_masks            uint8 [rows], or empty when all zero: fields awaiting a defaults card
```

Column descriptors are sorted by `(slot, kind)`. A dense column has `count == rows` and implicitly addresses rows
`0..rows-1`. A constant column has `count == 1`; its one value applies to every row, with no repeated values in the file.
This commonly removes entire buffers for default coordinate systems, constraints, offsets and zero coordinate components.
Constants are compared bit for bit, preserving negative zero. An optional field writes only the rows that have a value;
its sparse row slice has exactly `count` entries.
Missing entries are blank. An integer/real alternative uses two columns on the same slot, with disjoint row sets; its
meaning therefore never depends on decimal punctuation. Variable-length lists use the same column scheme, and
`field_counts` preserves the end of each row. Values for one field are contiguous even when the cards have long lists.
Unused integer or float buffers have length zero. No text copy of numeric model data is stored.

Slot numbers and their meaning form the version 1 model contract. The included catalogue describes fixed fields,
alternatives, forms, repeated groups and sections using the neutral names of the `.oos` catalogue. Changing a slot's
meaning or interpretation requires a new model schema version. Slot padding is not a value and is never serialized as
text. For `UNKNOWN` cards only, slot 1 holds the exact original card name, and later slots hold its uninterpreted field
texts. Unknown cards can therefore be exported unchanged without guessing their types.

Model reading rebuilds card tables from typed values in batches of 4096 rows, without formatting or parsing normal
numeric fields. It rebuilds identifier indexes and preserves card keys, source precision, comments, source markers,
placements and the part hierarchy. Source paths are descriptive: all card data is inside the archive, and no Include
reads a file on disk. The model reader validates shapes, types, offsets, dictionary codes, row ordering and overlapping
alternative fields before using them.

The optional document vectors have matching lengths and unique names within each part. Earlier version 4 archives
without a `documents` group remain readable. Applications can store a versioned optimization study and optimizer
state here; the model writer carries documents through copy-on-write edits and archive rewrites. Documents are
independent of card slots and are not part of `.oos` model text. A `.oos` write reports each omitted document.

`default_masks` preserves fields still awaiting a defaults card added by a later edit. For `Node`, bits 0 to 3 mean
position frame, displacement frame, permanent constraints and superelement. For `Bar2` and `Beam2`, bits 0 to 4 mean
property, orientation components 1 to 3 and offset kind. Other card types have an empty mask buffer.

### Structured control records

Control sections are small compared with the mesh. Their neutral names, values and options are stored as structured
records rather than a complete model text. Each control group has `kinds` (`uint8`, 1 record, 2 Include begin, 3 Include
end, 4 comments), `files` (`uint32`, included file index), `comments` (string vector), `value_offsets` (`uint64`, N+1)
and `values` (string vector, concatenated record fields). Order and source markers are retained. Unsupported program
statements that the `.oos` writer would omit cause archive writing to fail, preventing silent data loss.

Source files named by Include records are represented by their cards and source metadata. External files merely named
by other cards or control records, such as auxiliary data files, are retained as references; their payloads are not bundled.

### Compression and selective reads

Model buffers of at least 4096 bytes use shuffle and deflate (level 4 by default) with chunks of at most about one MiB.
Smaller buffers are contiguous. Compression level 0 leaves datasets contiguous. The writer processes one card type at
a time; its temporary buffers do not contain all model types or any result values.

For example, a viewer can read a part's `card_types`, select `Node`, read that table's column descriptors and load only
the coordinate slices in `reals`. The `ids` array identifies their rows. Fields supplied by defaults cards are written
explicitly so model reconstruction is independent of card type order. Opening the archive and listing result tables
never reads these buffers. This organization provides selective access; actual speed depends on model size, the HDF5
implementation, compression and storage.

## Archive APIs

```js
results.attach(deck, false);
results.writeArchive("/work/complete.ooh5"); // model plus results
const archive = oor.openResults("/work/complete.ooh5");
const model = archive.model();              // load and attach on demand
const independent = oor.readModel("/work/complete.ooh5");
model.writeArchive("/work/model-only.ooh5");
model.writeModel("/work/export.oos");
```

In C++, pass `ArchiveWriteOptions::model` to either `writeArchive` or `beginArchive`. The model is written when the
session begins; result tables can then be pushed as before. The API also includes an attached deck automatically and
preserves an embedded model when rewriting an archive that has not loaded its model. All archive writing uses a
temporary file and replaces the target only on successful completion.

The lower-level C++ interface can write a model alone without creating a facade result set:

```cpp
#include "oor/api/Deck.hpp"
#include "oor/results/archive/ArchiveModel.hpp"
#include "oor/results/archive/ArchiveWriter.hpp"

int main()
{
    const auto deck = oor::api::Deck::readModel("plate.oos");
    oor::results::ArchiveWriteOptions options;
    options.model = deck->model();
    const auto report = oor::results::writeArchive("model.ooh5", oor::results::ResultTables{}, options);
    const auto model = oor::results::archive::readModel("model.ooh5");
    return report.finished && model->cardCount() == deck->cardCount() ? 0 : 1;
}
```

For a streaming write, pass the same options to `beginArchive`, push existing `ResultTables` or individual tables,
then call `finish`. The model is serialized at session creation. See the [API declarations](api.rst)
and the [JavaScript guide](wasm-api.md#a-model-and-results-in-one-archive) for the application interfaces.

## Result tables

### `/index/tables`

| Column | Type | Meaning |
| --- | --- | --- |
| `group` | string | the group of the table: `/data/tables/000001` |
| `kind` | string | `displacements`, `singlePointConstrForces`, `stress`, `strain`, `forces`, `strainEnergy`, `eigenvalues`, `nodePointForces`, ... (the names of `ResultKind`) |
| `variant` | string | `""` for the results of an analysis; `psd`, `ato`, `crm`, `rms`, `no`, `pk`, `modalContribution` |
| `container` | string | the element type of an element table (`ShellQuad4`), the variant of a grid point stress table (`surface`); else `""` |
| `layout` | string | `centre` or `corner` for the element types that have both; else `""` |
| `table_name` | string | the name the table was given when it was made (free text) |
| `subcase`, `analysis_code`, `superelement`, `design_cycle`, `load_set` | int64 | where the table belongs (`-1`: none) |
| `sort` | int64 | 1 (one record a step) or 2 (one record a row) in the source |
| `step_kind` | string | `none`, `loadSet`, `mode`, `frequency`, `time`, `loadFactor` |
| `value_type` | string | `float32`, `float64`, `complex64`, `complex128` |
| `stored_format` | string | `real`, `realImaginary`, `magnitudePhase` (how the source wrote complex values; the archive always holds real and imaginary parts) |
| `status` | string | `validated`, `notValidated`, `experimental` |
| `stored_frame`, `frame_system` | string, int64 | the coordinate frame of the values |
| `word_size` | int64 | precision of the source: 4 (single) or 8 (double) |
| `units`, `title`, `subtitle`, `label` | string | texts of the table |
| `step_count`, `row_count`, `component_count` | int64 | the shape of `values` |
| `step_offset` | int64 | where the steps of the table start in `/index/steps` |
| `has_eigenvalues`, `has_eigenvalues_imag`, `has_cycles` | int64 | 1 when the table has those step labels |

### `/index/named`

`group`, `kind`, `name`, `table_name`, `subcase`, `analysis_code`, `superelement`, `design_cycle`, `status`,
`word_size`, `title`, `subtitle`, `label`, with the same meanings.

Named arrays live under the indexed group as typed datasets with their declared shapes. Text columns use the
archive's string-vector codec. `results.named()` returns the lightweight index and `results.readNamed(index)` reads
one named payload. `results.addNamed(input)` validates its kind, array shapes and text lengths against
`oor.namedResultCatalogue()` before writing. Static aeroelastic trim uses named groups for pressures, forces,
variables, stability derivatives, hinge moments and monitor loads. Structural tables remain under `/index/tables`.
The `analysis_code` values 101, 103, 105 and 144 identify static, modes, buckling and static aeroelastic output.

## Reading an archive without this library

With Python and h5py, list result tables only when the optional result index exists:

```python
import h5py

with h5py.File("run.ooh5", "r") as archive:
    if "index/tables" in archive:
        index = archive["index/tables"][...]
        for row in index:
            print(row["kind"].decode(), row["container"].decode(), row["subcase"], row["step_count"])
        first = index[0]["group"].decode()
        one_step = archive[first]["values"][0]     # reads and inflates the first step only
        components = [c.decode() for c in archive[first]["components"][...]]
```

To read only the first coordinate field of a native `Node` table, decode its small descriptors and slice the real
buffer. This example requires a root-part `Node` table and handles dense, constant and sparse columns:

```python
import h5py
import numpy as np

def strings(group):
    offsets = group["offsets"][:]
    data = bytes(group["bytes"][:])
    return [data[int(first):int(last)].decode("utf-8") for first, last in zip(offsets[:-1], offsets[1:])]

with h5py.File("model.ooh5", "r") as archive:
    part = archive["model/parts/0"]
    table = part[f"cards/{strings(part['card_types']).index('Node')}"]
    ids = table["ids"][:]
    slots = table["columns/slots"][:]
    kinds = table["columns/kinds"][:]
    column = int(np.flatnonzero((slots == 3) & (kinds == 2))[0])  # Node position1: real slot 3
    start = int(table["columns/value_offsets"][column])
    count = int(table["columns/counts"][column])
    row_start = int(table["columns/row_offsets"][column])
    values = table["reals"][start:start + count]                  # only this coordinate column
    dense = (1 << 64) - 1
    if row_start == dense - 1:
        x = np.full(len(ids), values[0])                         # one constant for every row
    elif row_start == dense:
        x = values
    else:
        rows = table["row_indices"][row_start:row_start + count]
        x = np.full(len(ids), np.nan)
        x[rows] = values
    print(list(zip(ids[:5], x[:5])))
```

These are coordinates in each node's position frame. Applying coordinate transformations requires the model's frame
data. The example marks absent fields as NaN; reconstructing full cards also requires applying catalogue defaults.
The full catalogue at `/model/schema/card_layouts` describes the slot meanings for other fields and card types.

## Measured native example

A local Windows release build (`-O3`, GCC) was measured on 9 November 2026 using 500,000 synthetic `Node` cards, no result
tables, compression level 4 and shuffle enabled. For node i (1 through 500,000), coordinates were
`[i * 0.00123456789, (i % 100) * 0.0123456789, (i % 7) * 0.125]`. Read times are medians of three full model reads after
one warm-up per format, including model reconstruction and identifier indexes.

| Measurement | `.oos` | `.ooh5` v4 |
| --- | --- | --- |
| File size | 24,643,633 bytes (24.64 MB) | 338,896 bytes (0.339 MB) |
| Native model read | 567 ms | 218 ms |

This regular data compresses particularly well. These measurements describe this synthetic case, not guaranteed ratios
for other models or browser runtimes. Control sections, source precision, irregular numeric values and result payloads
change file size and loading cost.

## Query tables (custom outputs)

The tables of the named queries (margins, envelopes, section cuts, any query) are written by
`oor::query::writeQueryTables` ([Hdf5Export.hpp](../include/oor/query/table/Hdf5Export.hpp)) to an HDF5 file of
this layout:

```text
/                                        attributes: format = "oor-query-tables", format_version = "1", producer
/tables/<name>/                          attribute row_count
/tables/<name>/column_names              strings: the columns, in order
/tables/<name>/columns/<column>          int64 (whole numbers), float64, or strings: one value a row
/tables/<name>/columns/<column>.offsets  int64 [rows + 1]: for a list column, row i holds values [offsets[i], offsets[i+1])
/tables/<name>/columns/<column>.valid    uint8 [rows]: when the column has nulls, 1 a value and 0 a null
```

## Versions

`format_version` changes when a reader of an earlier version could not read the file correctly. A reader refuses a
version it does not know (`IoError`), so an old viewer never shows wrong values from a newer file. Adding a dataset
that readers can ignore does not change the version.

| Contract | Current writes | Accepted reads | Purpose |
| --- | --- | --- | --- |
| Root archive `format_version` | `4` | `3`, `4` | Model/results container and result layout |
| `/model` `format_version` | `1` | `1` when a model is requested | Native model slot layout and interpretation |
| Table-package data contract | `75` | Checked by the receiving module | Query-table transport between Reader and Solver |

Archive and model version constants are in `ArchiveFormat.hpp`; the table-package contract is in
`include/oor/model/DataContract.hpp`. Changing a native slot's meaning requires a new model schema version. A change
that an earlier archive reader cannot handle also requires a new root archive version. Keep the schema, compatibility
checks, tests and documentation aligned.

Version 3 uses the same descriptive, case-sensitive element types as model records (`ShellQuad4`, `Bar2`,
`SolidHexa`). Result variants append their layout to that root (`ShellQuad4Composite`, `Bar2LoadStations`).
Other kind, step, component and index names keep their existing spelling.

Version 4 adds the optional native model described above. Result datasets retain the version 3 layout. This reader
accepts versions 3 and 4; versions 1 and 2 and unknown newer versions are refused. New writes always use version 4.
An older version 3 reader rejects version 4 instead of silently treating a model-bearing archive as a result-only file.
