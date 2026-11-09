# Element names

An element has one descriptive, case-sensitive name throughout Reader and Solver. Its model record, table metadata,
result container, diagnostic and formulation selector use the same root: `ShellQuad4`, for example.

## Build a name

Use the physical family first, then the shape and the node count when the topology fixes that count. Put a meaningful
variant last: `ShellQuad4Drilling` is a four-node quadrilateral shell with a drilling rotation. `SolidHexa` accepts
either supported interpolation order, so its name does not imply a fixed number of nodes.

| Family | Names |
| --- | --- |
| Shells | `ShellQuad4`, `ShellQuad8`, `ShellTria3`, `ShellTria6` |
| Shells with drilling rotation | `ShellQuad4Drilling`, `ShellTria3Drilling` |
| Other shells | `ShellConical2`, `ShellAxisym`, `ShearPanel4` |
| Solids | `SolidHexa`, `SolidPenta`, `SolidTetra`, `SolidPyramid` |
| Lines | `Rod2`, `Rod2InlineSection`, `Tube2`, `Bar2`, `Beam2`, `Beam3`, `CurvedBeam2` |
| Connectors | `Bushing`, `Bushing1D`, `Bushing2D`, `Gap2`, `Fastener`, `Weld`, `Seam`, `LineInterface` |
| Concentrated mass | `PointMassMatrix`, `PointMassInertia` |
| Springs, dampers and scalar masses | `SpringDofProperty`, `SpringDofInline`, `SpringScalarProperty`, `SpringScalarInline`; the same variants for `Damper` and `Mass` |
| Explicit spring/damper variants | Append `Explicit`, as in `SpringDofPropertyExplicit` |
| Plane stress/strain | `PlaneStressTria3`, `PlaneStressQuad4`, `PlaneStressTria6`, `PlaneStressQuad8`; the same shapes for `PlaneStrain` |
| General plane strain | `PlaneStrainQuad` |
| Axisymmetric solids | `AxisymQuad`, `AxisymQuad4`, `AxisymQuad8`, `AxisymTria`, `AxisymTria6` |
| Cohesive interfaces | `CohesiveQuad`, `CohesiveAxisymQuad`, `CohesiveHexa`, `CohesivePenta` |
| Acoustic/fluid elements | `AcousticAxisymFluid2/3/4`, `AxisymFluid2/3/4`, `AcousticInfiniteTria3`, `AcousticInfiniteQuad4`, `AcousticSlot3/4` |
| Acoustic absorbers/barriers | `AcousticAbsorber`, `AcousticAbsorberHexa`, `AcousticBarrierHexa` |
| Other families | `AeroQuad4`, `AeroTria3`, `CrackTip2D`, `CrackTip3D`, `GeneralMatrix`, `HeatCapacity2`, `ViscousDamper2`, `UserDefined1` through `UserDefined9` |

`Dof` names a connection between individual degrees of freedom; `Scalar` names a connection between scalar points.
`Property` refers to a separate property record. `Inline` puts the coefficient or section on the element record.
`PointMassMatrix` provides the full mass matrix; `PointMassInertia` provides mass, offset and inertia.

The [card catalogue](model-format-cards.md) gives every record's exact spelling and values, including auxiliary records,
plot-only elements, heat-transfer surfaces and module connectors. Being readable by the reader does not imply support
for an analysis: inspect the solver's support report.

## Use the root consistently

| Role | Example |
| --- | --- |
| Model record and result `elementType` | `ShellQuad4` |
| Reader data/table/format | `ShellQuad4Card`, `ShellQuad4Table`, `ShellQuad4Format` |
| Solver element family | `ShellQuad4Element` |
| Numerical kernel and geometry | `ShellQuad4`, `ShellQuad4Geometry` |
| Drilling family and geometry | `ShellQuad4DrillingElement`, `ShellQuad4DrillingGeometry` |
| Formulation parameter | `ShellQuad4Formulation` |
| Result layout variant | `ShellQuad4Composite`, `Bar2LoadStations` |
| Source directory and function | `shellQuad4/`, `shellQuad4DesignFields()` |

Derived directory and function names use a lower-case first letter. Exposed element type values retain
their exact capitalized spelling. A result layout suffix describes the result rows; it does not create another model
card type. Formulation names remain separate from the element type.

Native model records and table packages use this vocabulary directly.

## Compatibility

Reader and Solver are version `0.2.0`. Build and update them together. Table packages use data contract **75**.
New `.oorh5` archives use format version **4** and native model schema **1**; the reader also accepts version **3**
result archives. Version 4 retains version 3 result layouts and element names while adding native model columns.
Older version 3 readers reject version 4 files. See [archive compatibility](archive-format.md#versions).

Earlier element names have no aliases. Update model records, API selectors, result metadata and any dependent client
to the current vocabulary before use. Incompatible table packages and unknown archive or model schema versions are refused.
