# The HDF5 results archive (format version 3)

The results archive is the file format this library writes for analysis results and for custom outputs. It is an
ordinary HDF5 file with a layout of our own: any HDF5 tool (h5py, h5dump, HDFView, h5wasm, ...) can open it, and
nothing in it follows the layout of another software vendor's files.

The names below are spelled once in the code, in
[include/oor/results/archive/ArchiveFormat.hpp](../include/oor/results/archive/ArchiveFormat.hpp). The writer is
[src/results/archive/ArchiveWriter.cpp](../src/results/archive/ArchiveWriter.cpp), the reader
[src/results/archive/ArchiveResultFile.cpp](../src/results/archive/ArchiveResultFile.cpp).

## Why this layout

The archive is designed for a web viewer that opens a file once and then shows one table, one step at a time:

| Need | Choice |
| --- | --- |
| Open a large file fast | Everything needed to list the tables is in a few small datasets under `/index`: one read each, no walk over thousands of groups. |
| Show one step of one table | The values of a table are one dataset `(steps, rows, components)` chunked by whole steps: one step is read and inflated alone. |
| Small files | Shuffle + deflate on every dataset of more than 1024 values (level 4 by default, 0 to 9), and the precision of the source is kept: single precision values stay `float32`. |
| Readable by anyone | Plain HDF5 types only (integers, floats, UTF-8 strings), plain English names, enumerations stored as their names (`"time"`, `"float32"`), never as codes. |

## Layout

```text
/                                attributes (fixed-length UTF-8 strings):
                                   format          "oor-results-archive"
                                   format_version  "3"
                                   producer        "oor_reader 0.1.0"
                                   created_at      "2026-11-05T10:00:00Z"
                                   title, subtitle, label
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

## Reading an archive without this library

With Python and h5py:

```python
import h5py

with h5py.File("run.oorh5", "r") as archive:
    index = archive["index/tables"][...]
    for row in index:
        print(row["kind"].decode(), row["container"].decode(), row["subcase"], row["step_count"])
    first = index[0]["group"].decode()
    one_step = archive[first]["values"][0]          # reads and inflates the first step only
    components = [c.decode() for c in archive[first]["components"][...]]
```

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

Version 3 uses the same descriptive, case-sensitive element types as model records (`ShellQuad4`, `Bar2`,
`SolidHexa`). Result variants append their layout to that root (`ShellQuad4Composite`, `Bar2LoadStations`).
Other kind, step, component and index names keep their existing spelling. Earlier archive versions are refused.
