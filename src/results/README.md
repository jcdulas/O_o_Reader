# `results`

Native `.oorh5` model/results archives; result tables from an archive or a program (tables made in memory);
attaching results to a model; the Excel workbook of results (an output).

- Headers: `include/oor/results/`. Sources: this folder. Tests: `tests/cpp/results/`.
- May use: `model`, `deck`, `io`, `base` (checked by `tools/check_layering.py`).
- Start reading at `core/ResultFile.hpp` (what every result file offers), then `core/ResultTable.hpp` (one table: lazy,
  cached, shape (steps, rows, components)), `core/ResultTables.hpp` (the containers by kind and element type) and
  `core/ResultCatalogue.hpp` (the components of each kind of table; its data in `core/ResultCatalogueData.cpp`).
- `core/ReadResults.cpp`: opens a results archive, recognised by its content.
- `core/MemoryResults.hpp`: tables made in memory by a program, checked against the catalogue.
- `archive/`: the HDF5 model/results archive, writer (`ArchiveWriter.cpp`) and reader (`ArchiveResultFile.cpp`);
  `ArchiveModel.cpp` stores native typed model columns and loads them on demand. Its format is described in
  [docs/archive-format.md](../../docs/archive-format.md). `ArchiveWriteOptions::model` includes a model in a one-shot
  write or a streaming session. `ArchiveResultFile::hasModel()` lists its presence without loading its values;
  `readModel()` reconstructs it independently of result values. New archives use version 4; version 3 results remain readable.
- `xlsx/`: the Excel workbook of results (`XlsxWriter.cpp`), on the ZIP container of `io/zip`.
- `attach/`: links the rows of the tables to the cards of a model.
