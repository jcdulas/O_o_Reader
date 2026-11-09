# Validation models

Model files (`.oos`, [docs/model-format.md](../../docs/model-format.md)) written for this project, by theme (one folder
each), that the library must read without an error:

- `tests/cpp/model/ValidationModelsTest.cpp` reads every main model file of this folder (a `.oos` file that no
  `Include` record of another file names), with the files it includes, and checks that no error diagnostic comes out;
- `tests/cpp/model/ModelFileRoundTripTest.cpp` writes each model back as model files with its Include structure, reads
  them again and checks that the card tables are the same and that the files written are a fixed point.

A file that other models include (a mesh shared by the models of a folder, such as `plate_model.oos`) is read with
them, not alone. The other files next to the models (`.csv`, `.impf`) are data files that a model names.

To run a model with a solver that reads the Nastran input format, convert it with O_o_Converter:
`ooc export <model>.oos <model>.bdf --split-includes`.

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
