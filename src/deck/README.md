# `deck`

The O_o model record format: records separated by newlines, fields separated by `;`, comments and Include files.

- `records/RecordText.hpp`: split and format one record, including quoted values.
- `records/ModelFileLexer.hpp`: read records, section lines, comments and Include files with source locations.
- `raw/`: the neutral card names and slot blocks used between the record layouts and the card readers.
- `includes/`: Include depth and memory limits, and output paths for included model files.
- `writer/`: staged file writes; a failed write leaves existing files unchanged.

May use: `io`, `base`. The model format is specified in `docs/model-format.md`.
