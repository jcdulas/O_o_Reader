# O_o_Reader documentation

Read and edit `.oos` model records and native `.oorh5` model/results archives, query model data, and export results.

**[Read the public HTML site](https://jcdulas.github.io/O_o_Reader/)**

## Start here

- [Getting started](docs/getting-started.md): build requirements and a first working example.
- [Project overview](README.md): capabilities, repository layout and build commands.
- [API reference](docs/api.rst): the current JavaScript types and C++ entry points, included directly from source.
- [JavaScript optimization](docs/optimization.md): design points, derivatives and model updates.
- [Architecture with diagrams](docs/architecture.md): module dependencies, data ownership and execution flow.
- [Module guide](docs/modules.md): implementation structure and detailed technical guides.
- [Element names](docs/element-naming.md): shared vocabulary, implementation roles and compatibility.
- [Download or build the documentation](docs/documentation.md): published site, HTML archives and local builds.

## Detailed guides

- [Model format](docs/model-format.md) and [card catalogue](docs/model-format-cards.md).
- [JavaScript guide](docs/wasm-api.md) and [native model/results archive](docs/archive-format.md).
- [Architecture](docs/architecture.md), [table packages](docs/table-package.md) and [naming](docs/naming.md).

## Reader and Solver

| Project | Responsibility |
| --- | --- |
| [O_o_Reader](https://github.com/jcdulas/O_o_Reader/blob/main/index.md) | Model records, native model/results archives and model queries |
| [O_o_Solver](https://github.com/jcdulas/O_o_Solver/blob/main/index.md) | Finite element analyses from reader tables |

The reader and solver have public documentation sites and exchange model data through typed table packages.

~~~{toctree}
:hidden:
:maxdepth: 2
:caption: User guide

docs/getting-started
docs/optimization
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
