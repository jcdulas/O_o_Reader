# CLAUDE.md: working on O_o_Reader

O_o_Reader is a C++20 library, compiled to WebAssembly, that reads and writes O_o model files (`.oos`),
stores results and custom outputs in an HDF5 archive of its own layout, writes results as Excel workbooks, and runs named
queries on a model and its results. Start with [README.md](README.md) and [docs/architecture.md](docs/architecture.md).

## Rules

1. **Legal** ([LEGAL.md](LEGAL.md)): no binary result format or result database of another vendor; no vendor or
   product name in code, comments, messages, tests or file formats ("Nastran" only to describe the separate converter's input format);
   never copy or quote the documentation of other software, never add it to the repository, never take test data from
   its examples. Test decks and samples are written for the project. Keep the history of the code out of the
   repository and of the commit messages.
2. **English** for everything in the repository: code, names, comments, messages, docs, tests, commit messages. The
   owner writes in French: answer in French in the conversation.
3. **Readable by an intermediate developer** (about five years of experience): plain C++, small functions, comments
   that say why, no clever template tricks where a loop does.
4. **Layering** (`tools/check_layering.py`): `base` → `io` → `deck` → `model` → `results` → `analysis` → `query` →
   `api`; a module uses only those before it. libhdf5 headers only in `src/io/hdf5`, operating-system headers only in
   `src/base/platform`.
5. **C++20 within what GCC 12 has**, `-Wall -Wextra -Wpedantic -Werror` natively. Names: types `CamelCase`, functions
   and variables `camelCase`, members `m_name`, constants `UPPER_CASE`; `.clang-format` (4 spaces, 120 columns).
6. **WebAssembly is 64-bit (Memory64) and single-threaded**: every object, libhdf5 and zlib included, is built with
   `-sMEMORY64=1`; code that starts threads goes through `base::ThreadPool`, which has zero workers
   in the WebAssembly build.
7. **File formats**: the archive layout is spelled in `include/oor/results/archive/ArchiveFormat.hpp` and described in
   `docs/archive-format.md`; a change that an earlier reader cannot read raises `FORMAT_VERSION`. The Excel workbook
   (`src/results/xlsx/`) is an output only.

## Build and test

Native (Windows: MSYS2 UCRT64 first on the PATH, `C:\msys64\ucrt64\bin`):

```bash
scripts/check.sh                  # build, C++ tests, layering
scripts/check.sh xlsx             # only the tests whose name matches
```

WebAssembly (Emscripten SDK on the PATH; libhdf5 for WebAssembly built once with `scripts/build_hdf5_wasm.sh`):

```bash
scripts/build_wasm.sh <work folder>
node --test wasm/test/smoke.test.mjs
```

## Way of working

- Commits on the owner's request only.
- Commit messages must be short, factual and neutral. Describe the project change without references to software
  vendors, products, AI tools or model names, comparisons, or claims about legal compliance.
- Do not append automatic tool-attribution or co-author trailers.
- Line endings LF (`.gitattributes`).
