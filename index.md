# O_o_Reader documentation

Read and edit model records, query model data, and manage results.

## Start here

- [Getting started](docs/getting-started.md): build requirements and a first working example.
- [Project overview](README.md): capabilities, repository layout and build commands.
- [API reference](docs/api.rst): the current JavaScript types and C++ entry points, included directly from source.
- [Module guide](docs/modules.md): implementation structure and detailed technical guides.
- [Element names](docs/element-naming.md): shared vocabulary, implementation roles and compatibility.
- [Download or build the documentation](docs/documentation.md): private HTML artifacts and local builds.

## Detailed guides

- [Model format](docs/model-format.md) and [card catalogue](docs/model-format-cards.md).
- [JavaScript guide](docs/wasm-api.md) and [results archive](docs/archive-format.md).
- [Architecture](docs/architecture.md), [table packages](docs/table-package.md) and [naming](docs/naming.md).

## The three projects

| Project | Responsibility |
| --- | --- |
| [O_o_Reader](https://github.com/jcdulas/O_o_Reader/blob/main/index.md) | Model records, model queries and result storage |
| [O_o_Solver](https://github.com/jcdulas/O_o_Solver/blob/main/index.md) | Finite element analyses from reader tables |
| [O_o_Converter](https://github.com/jcdulas/O_o_Converter/blob/main/index.md) | External input text and .oos interchange |

The source pages and the generated HTML are kept in the private repositories. An authenticated account with access
to the repository is required to download its documentation artifact. No public site is deployed.

~~~{toctree}
:hidden:
:maxdepth: 2
:caption: User guide

docs/getting-started
README
docs/model-format
docs/model-format-cards
docs/wasm-api
docs/archive-format
docs/table-package
docs/naming
docs/element-naming
~~~

~~~{toctree}
:hidden:
:maxdepth: 2
:caption: Reference and development

docs/api
docs/architecture
docs/modules
validation/models/README
docs/documentation
LEGAL
~~~
