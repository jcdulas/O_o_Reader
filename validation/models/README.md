# Validation models

Model files (`.oos`, [docs/model-format.md](../../docs/model-format.md)) written for this project, by theme (one folder
each), that the library must read without an error:

- `tests/cpp/model/ValidationModelsTest.cpp` reads every main model file of this folder (a `.oos` file that no
  `Include` record of another file names), with the files it includes, and checks that no error diagnostic comes out;
- `tests/cpp/model/ModelFileRoundTripTest.cpp` writes each model back as model files with its Include structure, reads
  them again and checks that the card tables are the same and that the files written are a fixed point;
- `tests/cpp/results/ArchiveModelTest.cpp` writes every main model to native `.oorh5` columns, reloads it and compares
  both its card tables and exported `.oos` text. Additional cases cover exact real values, comments, source metadata,
  parts, persistent keys, pending defaults, empty models and reading after source Include files have been removed.

A file that other models include (a mesh shared by the models of a folder, such as `plate_model.oos`) is read with
them, not alone. The other files next to the models (`.csv`, `.impf`) are data files that a model names.

The native model archive schema and its compatibility rules are described in
[docs/archive-format.md](../../docs/archive-format.md). Keep validation models as readable `.oos` sources; archive
tests generate their binary fixtures in temporary directories.

To run a supported model, load its `.oos` records with O_o_Reader and pass its typed table packages to O_o_Solver.

Rules for a model added here:

1. **Written for the project.** Never copied, in whole or in part, from a vendor's documentation, examples or test
   files, not even into a comment ([LEGAL.md](../../LEGAL.md)).
2. **Small.** A few elements to a few hundred: each model reads in a fraction of a second.
3. **Physically consistent.** Real geometry, properties and materials that exist, constraints that remove the rigid
   body motion of a static analysis, loads that the case section selects. Units: mm, N, tonne, s (W and degrees C for
   heat transfer).
4. **Commented.** A comment at the top says what the model exercises.

The folders whose name starts with `read_only_` hold cards that are read and written by the library but would not make
a runnable analysis on their own.
