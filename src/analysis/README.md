# `analysis`

Geometry, frames, sections, laminates, mass, quality, adjacency, components, connections, validation.

- Headers: `include/oor/analysis/`. Sources: this folder. Tests: `tests/cpp/analysis/`.
- May use: `results`, `model`, `deck`, `io`, `base` (checked by `tools/check_layering.py`).
- Start reading at `geometry/ElementGeometry.hpp` (geometry) and `mass/MassCalculator.hpp` (mass, sections and laminates).
- Mesh quality and adjacency (`quality/`, `adjacency/`): start at `quality/MeshQuality.hpp` and `adjacency/MeshAdjacency.hpp`.
- Validation (`validation/`): start at `validation/engine/CheckRegistry.hpp` and `validation/engine/ValidationEngine.hpp`; one class and one file per check in the folder of its family.
- Components and connections (`components/`, `connections/`): start at `components/ComponentModel.hpp` (components, boundaries, stamps), `components/GroupSet.hpp` (groups and zones) and `connections/FastenerPiercings.hpp` (where CFAST and CWELD pierce the shells, private helpers in `connections/fasteners/`), then `connections/Spiders.hpp` and `connections/ConnectionClusters.hpp`, and `connections/ContactSearch.hpp` (contact between bodies: `ContactBodies`, `ContactPairs`, `ContactSurfaces`, private helpers in `connections/contact/`).
