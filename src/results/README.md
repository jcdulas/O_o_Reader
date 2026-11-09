# `results`

Result tables, whatever they come from: the HDF5 results archive or a program (tables made in memory); attaching
results to a model; the Excel workbook of results (an output).

- Headers: `include/oor/results/`. Sources: this folder. Tests: `tests/cpp/results/`.
- May use: `model`, `deck`, `io`, `base` (checked by `tools/check_layering.py`).
- Start reading at `core/ResultFile.hpp` (what every result file offers), then `core/ResultTable.hpp` (one table: lazy,
  cached, shape (steps, rows, components)), `core/ResultTables.hpp` (the containers by kind and element type) and
  `core/ResultCatalogue.hpp` (the components of each kind of table; its data in `core/ResultCatalogueData.cpp`).
- `core/ReadResults.cpp`: opens a results archive, recognised by its content.
- `core/MemoryResults.hpp`: tables made in memory by a program, checked against the catalogue.
- `archive/`: the HDF5 results archive, writer (`ArchiveWriter.cpp`) and reader (`ArchiveResultFile.cpp`); its format
  is described in `docs/archive-format.md`.
- `xlsx/`: the Excel workbook of results (`XlsxWriter.cpp`), on the ZIP container of `io/zip`.
- `attach/`: links the rows of the tables to the cards of a model.
