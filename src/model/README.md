# `model`

The model: cards, column storage, containers, handles, editing, control sections.

- Headers: `include/oor/model/`. Sources: this folder. Tests: `tests/cpp/model/`.
- May use: `deck`, `io`, `base` (checked by `tools/check_layering.py`).
- Start reading at `core/Model.hpp`.
- The Case Control is in `control/case_control/`: `CaseControl`, `Subcase`,
  `CaseEntry`, `CommandCatalogue` and the family functions of `commands/`, the parser and the writer.
- The Executive Control and the File Management are in `control/executive_control/` and `control/file_management/`, on
  the statement classes shared in `control/statement/`: `ExecutiveControl`,
  `FileManagement`, `ControlStatement`, `StatementParameter`, the statement definitions, the reader and the writer.
- The design model is in `design/` (`DesignModel`, `applyDesign`, and the registry of the card types a design relation can drive,
  `DesignableCards.cpp`; the functions of each card type are next to the card, `<Card>DesignFields.cpp`).
- The model file (`.oos`): `core/ModelFileReader.hpp` and `core/ModelFileWriter.hpp`; the card layouts that map the
  fields of a record to the slots a card reader reads, `format/CardLayouts.hpp` (the catalogue
  `formats/oos-layouts.json`, compiled in; made by `tools/oos_layouts/`, listed in `docs/model-format-cards.md`); the
  records of the control sections, `control/ControlRecords.hpp`.
