#include "oor/results/core/NamedResultKinds.hpp"

#include <array>

namespace oor::results
{

namespace
{

constexpr auto WEIGHT = std::to_array<NamedArrayInfo>({
    {"referencePoint", "int64", "", "the grid point the weight is reported about (0: the origin of the basic system)"},
    {"mo", "float", "(6, 6)", "rigid body mass matrix about the reference point"},
    {"s", "float", "(3, 3)", "transformation from the basic axes to the principal mass axes"},
    {"mass", "float", "(3)", "mass in each of the three principal directions"},
    {"cg", "float", "(3, 3)", "centre of gravity for each principal direction: a row of (x, y, z) a direction"},
    {"inertia", "float", "(3, 3)", "inertia matrix about the centre of gravity"},
    {"principalInertia", "float", "(3)", "principal moments of inertia"},
    {"principalAxes", "float", "(3, 3)", "principal axes of the inertia (a column an axis)"},
});

constexpr auto MATRIX = std::to_array<NamedArrayInfo>({
    {"shape", "int64", "(2)", "the number of rows and of columns"},
    {"row", "int32", "(n)", "the row of each stored term, from 0"},
    {"column", "int32", "(n)", "the column of each stored term, from 0"},
    {"value", "float or complex", "(n)",
     "the value of each term (real or complex, single or double precision as the type of the table says)"},
    // The labels of the degrees of freedom a matrix made in memory or read from DMIG cards carries: the point (a grid,
    // scalar or extra point, or a response or design variable) and component of every row and every column index.
    {"rowIds", "int32", "(rows)", "the identification number of each row (a point, a response, a variable)", false,
     true},
    {"rowComponents", "int32", "(rows)", "the component of each row (0 where there is none)", false, true},
    {"columnIds", "int32", "(columns)", "the identification number of each column", false, true},
    {"columnComponents", "int32", "(columns)", "the component of each column (0 where there is none)", false, true},
    // The harmonic number of a DMIAX degree of freedom (axisymmetric analysis), given with the four labels above.
    {"rowHarmonics", "int32", "(rows)", "the harmonic number of each row (-2147483648 where the field is blank)", false,
     true},
    {"columnHarmonics", "int32", "(columns)", "the harmonic number of each column (-2147483648 where blank)", false,
     true},
});

constexpr auto DESIGN_VARIABLES = std::to_array<NamedArrayInfo>({
    {"internalId", "int32", "(n)", "the internal number of the variable"},
    {"id", "int32", "(n)", "the DESVAR identification number"},
    {"lower", "float", "(n)", "the lower bound"},
    {"upper", "float", "(n)", "the upper bound"},
    {"moveLimit", "float", "(n)", "the move limit of one cycle (negative: the default fraction)"},
    {"unused", "float", "(n)", "the last word of the record (0)"},
    {"label", "text", "(n)", "the label of the variable", true},
});

constexpr auto DESIGN_HISTORY = std::to_array<NamedArrayInfo>({
    {"designCycle", "int32", "(n)", "the design cycle the table came after (-1: none)"},
    {"designIteration", "int32", "(n)", "the design iteration"},
    {"convergenceFlag", "int32", "(n)", "1 soft convergence, 2 hard convergence criteria"},
    {"convergenceResult", "int32", "(n)", "0 not converged, 1 soft, 2 hard, 3 and 4 best design"},
    {"objectiveInitial", "float", "(n)", "the objective at the start of the iteration"},
    {"objectiveFinal", "float", "(n)", "the objective at its end"},
    {"maxConstraint", "float", "(n)", "the value of the most violated constraint (negative: none violated)"},
    {"maxConstraintRow", "int32", "(n)", "the row of that constraint in the constraint table"},
    {"designVariableValues", "float", "(n, variables)", "the value of each design variable at the iteration"},
    {"convergence", "text", "(n)", "soft or hard: the criteria", true},
    {"result", "text", "(n)", "no, soft, hard or bestDesign: the outcome", true},
});

constexpr auto DESIGN_VARIABLE_HISTORY = std::to_array<NamedArrayInfo>({
    {"objective", "float", "(n)", "the objective of each design cycle"},
    {"approximateObjective", "float", "(n)", "the objective of the approximate model (0 where there is none)"},
    {"maxConstraint", "float", "(n)", "the largest constraint value of each cycle"},
    {"designVariableIds", "int32", "(variables)", "the DESVAR IDs, the columns of the values"},
    {"designVariableValues", "float", "(n, variables)", "the value of each design variable at each cycle"},
});

constexpr auto RESPONSES = std::to_array<NamedArrayInfo>({
    {"internalId", "int32", "(n)", "the internal number of the response"},
    {"id", "int32", "(n)", "the DRESP1 identification number"},
    {"responseType", "int32", "(n)", "the response type code (1 weight, 5 displacement, 6 stress, ...)"},
    {"region", "int32", "(n)", "the region of the response"},
    {"subcase", "int32", "(n)", "the subcase"},
    {"itemCode", "int32", "(n)", "the component or item code"},
    {"itemId", "int32", "(n)", "the grid point, element or property"},
    {"typeFlag", "int32", "(n)", "the flag word of the record"},
    {"superelementId", "int32", "(n)", "the superelement"},
    {"words", "int32", "(n, 15)", "the 15 words of the record as integers"},
    {"label", "text", "(n)", "the label of the response", true},
    {"typeName", "text", "(n)", "the name of the response type (weight, displacement, ...)", true},
});

constexpr auto RESPONSE_COLUMNS = std::to_array<NamedArrayInfo>({
    {"internalId", "int32", "(n)", "the internal number of the response"},
    {"id", "int32", "(n)", "the DRESP1 or DRESP2 identification number"},
    {"source", "int32", "(n)", "1 a DRESP1 response, 2 a DRESP2 response"},
    {"responseType", "int32", "(n)", "the DRESP1 type code; 0 for a DRESP2"},
    {"itemId", "int32", "(n)", "the element, grid point or mode of a DRESP1"},
    {"component", "int32", "(n)", "the component of a DRESP1"},
    {"subcase", "int32", "(n)", "the subcase"},
    {"extra", "int32", "(n)", "a ply, a view element or a RANDPS ID of a DRESP1; the dynamic flag of a DRESP2"},
    {"superelementId", "int32", "(n)", "the superelement"},
    {"frequency", "float", "(n)", "the frequency or time of a dynamic response (NaN: none)"},
});

// The values of the responses of a design cycle and the sensitivities of the responses to the design variables: what a
// solver that runs in memory gives. The queries of the optimisation read them with the other optimisation tables.

constexpr auto RESPONSE_VALUES = std::to_array<NamedArrayInfo>({
    {"designCycle", "int32", "(n)", "the design cycle the value belongs to"},
    {"id", "int32", "(n)", "the identification number of the response (DRESP1, DRESP2, DRESP3)"},
    {"subcase", "int32", "(n)", "the subcase (0: none)"},
    {"value", "float", "(n)", "the value of the response at the design cycle"},
    {"lower", "float", "(n)", "the lower bound of the constraint on the response (NaN: none)"},
    {"upper", "float", "(n)", "the upper bound of the constraint on the response (NaN: none)"},
    {"constraintSet", "int32", "(n)",
     "the identification number of the constraint set (DCONSTR) of the bounds (0: none)"},
    {"isObjective", "int32", "(n)", "1 for the response the optimisation minimises, else 0"},
    {"label", "text", "(n)", "the label of the response", true},
    {"responseType", "text", "(n)", "the type of the response (weight, displacement, stress, ...)", true},
    {"itemId", "int32", "(n)",
     "the grid point, element or mode the value is of, when a response has several items (0: none)", false, true},
    {"component", "int32", "(n)", "the component or item code of the value (0: none)", false, true},
    {"layer", "int32", "(n)", "the ply or layer of the value (0: none)", false, true},
});

constexpr auto SENSITIVITIES = std::to_array<NamedArrayInfo>({
    {"designCycle", "int32", "(n)", "the design cycle the sensitivity belongs to"},
    {"responseId", "int32", "(n)", "the identification number of the response"},
    {"subcase", "int32", "(n)", "the subcase of the response (0: none)"},
    {"designVariable", "int32", "(n)", "the identification number of the design variable (DESVAR)"},
    {"sensitivity", "float", "(n)", "the derivative of the response with respect to the variable, in their units"},
    {"itemId", "int32", "(n)", "the grid point, element or mode of the response item (0: none)", false, true},
    {"component", "int32", "(n)", "the component or item code of the response item (0: none)", false, true},
    {"layer", "int32", "(n)", "the ply or layer of the response item (0: none)", false, true},
});

// The aeroelastic tables. A result of these kinds has the Mach number, the dynamic pressure and the
// reference chord, span and area of the aerodynamic model among its attributes (`mach`, `dynamicPressure`,
// `referenceChord`, `referenceSpan`, `referenceArea`) when they are known.

constexpr auto FLUTTER_SUMMARY = std::to_array<NamedArrayInfo>({
    {"mode", "int32", "(n)", "the number of the mode (the curve of the V-g and V-f plots) a point belongs to"},
    {"point", "int32", "(n)", "the position of the point in its curve, from 1"},
    {"newCurve", "int32", "(n)", "1 for the first point of a curve, else 0"},
    {"mach", "float", "(n)", "the Mach number of the point"},
    {"densityRatio", "float", "(n)", "the density ratio of the point"},
    {"velocity", "float", "(n)", "the velocity of the point"},
    {"damping", "float", "(n)", "the damping of the mode at the point"},
    {"frequency", "float", "(n)", "the frequency of the mode at the point"},
});

constexpr auto TRIM_VARIABLES = std::to_array<NamedArrayInfo>({
    {"variableType", "int32", "(n)",
     "1 a rigid body variable (angle of attack, pitch rate, load factor), 2 a control surface"},
    {"statusCode", "int32", "(n)",
     "the status of the variable: 3 for a fixed variable, 1 and 2 for a free one (rigid body, control surface)"},
    {"isFixed", "int32", "(n)", "1 when the variable is fixed (status 3), 0 when it is free"},
    {"value", "float", "(n)", "the value of the variable (the trimmed value of a free one)"},
    {"variable", "text", "(n)",
     "the label of the variable (Intercept, AngleOfAttack, PitchRate, RigidAccelZ, ..., a control surface)", true},
    {"variableId", "int32", "(n)", "the AESTAT or AESURF of the variable, 0 for the intercept (when it is known)",
     false, true},
});

constexpr auto STABILITY_DERIVATIVES = std::to_array<NamedArrayInfo>({
    {"values", "float", "(n, 6, 6)",
     "the coefficients of each trim variable: the axis 1 is cx, cy, cz, cmx, cmy, cmz and the axis 2 rigid unsplined, "
     "rigid splined, elastic restrained, elastic unrestrained, inertial restrained, inertial unrestrained"},
    {"variable", "text", "(n)",
     "the trim variable (REFCOEFF for the reference coefficients, AngleOfAttack, PitchRate, ..., a control surface)",
     true},
});

constexpr auto HINGE_MOMENT_DERIVATIVES = std::to_array<NamedArrayInfo>({
    {"values", "float", "(n, 5)",
     "the hinge moment derivative of a control surface for each trim variable: rigid, elastic restrained, elastic "
     "unrestrained, inertial restrained, inertial unrestrained"},
    {"variable", "text", "(n)",
     "the trim variable (Intercept for the reference condition, AngleOfAttack, ..., a control surface)", true},
});

constexpr auto CONTROL_SURFACE_RESULTS = std::to_array<NamedArrayInfo>({
    {"position", "float", "(n)", "the trimmed position of the control surface"},
    {"positionLower", "float", "(n)", "its lower position limit"},
    {"positionUpper", "float", "(n)", "its upper position limit"},
    {"hingeMoment", "float", "(n)", "the hinge moment"},
    {"hingeMomentLower", "float", "(n)", "its lower limit (-1e10 where there is none)"},
    {"hingeMomentUpper", "float", "(n)", "its upper limit (1e10 where there is none)"},
    {"surface", "text", "(n)", "the label of the control surface", true},
});

constexpr auto AERO_PRESSURES = std::to_array<NamedArrayInfo>({
    {"elementId", "int32", "(n)", "the identification number of the aerodynamic element (box)"},
    {"pressureCoefficient", "float", "(n)", "the pressure coefficient"},
    {"pressure", "float", "(n)", "the aerodynamic pressure (the dynamic pressure times the coefficient)"},
    {"elementKind", "text", "(n)", "LS lifting surface, ZIB or YIB interference body, ZSB or YSB slender body", true},
});

constexpr auto AERO_FORCES = std::to_array<NamedArrayInfo>({
    {"nodeId", "int32", "(n)",
     "the number of the aerodynamic grid point (an internal number that follows the order of the pressure table when "
     "the solver gives no other)"},
    {"forces", "float", "(n, 6)", "the force and the moment at the point: t1 t2 t3 r1 r2 r3"},
    {"elementKind", "text", "(n)", "the kind of aerodynamic element (LS, ZIB, ZSB, YIB, YSB)", true},
    {"group", "int32", "(n)", "the group of the point (when it is known)", false, true},
});

// ---- what a solver produces in memory

// EPSILON and the external work of each static subcase: the checks of the static solution a solver gives.
constexpr auto EPSILON = std::to_array<NamedArrayInfo>({
    {"subcase", "int32", "(n)", "the subcase"},
    {"epsilon", "float", "(n)",
     "the error ratio of the static solution: the work of the residual load over the work of the load (the solver "
     "flags a "
     "value above 1e-3)"},
});

constexpr auto EXTERNAL_WORK = std::to_array<NamedArrayInfo>({
    {"subcase", "int32", "(n)", "the subcase"},
    {"externalWork", "float", "(n)", "the work of the load of the subcase on the displacements it causes"},
});

// The extreme of a component over the load cases, with the load case that gives it, without the rows of every load
// case: a row a grid point (or place in an element) and component.
constexpr auto ENVELOPE = std::to_array<NamedArrayInfo>({
    {"id", "int32", "(n)", "the grid point of a nodal result, the element of an element result"},
    {"nodeId", "int32", "(n)",
     "the grid point of a place in an element (a corner of a shell; 0 at the centre); the grid point of a nodal row"},
    {"layer", "int32", "(n)", "the layer of an element row (the fibre of a shell, the ply), from 1; 1 for a nodal row"},
    {"max", "float", "(n)", "the largest value over the load cases"},
    {"maxSubcase", "int32", "(n)", "the subcase that gives it"},
    {"maxStep", "float", "(n)", "the step of that subcase that gives it (a time, a frequency, a mode); NaN for none"},
    {"min", "float", "(n)", "the smallest value over the load cases"},
    {"minSubcase", "int32", "(n)", "the subcase that gives it"},
    {"minStep", "float", "(n)", "the step of that subcase that gives it; NaN for none"},
    {"result", "text", "(n)",
     "the result kind of the row: displacements, singlePointConstrForces, stress, strain, forces, ...", true},
    {"elementType", "text", "(n)", "the element type of an element row (ShellQuad4); empty for a nodal row", true},
    {"component", "text", "(n)", "the component or quantity the envelope is of (t3, magnitude, vonMises, oxx, ...)",
     true},
});

// The modal effective mass of the MEFFMASS output, a row a mode.
constexpr auto MODAL_EFFECTIVE_MASS = std::to_array<NamedArrayInfo>({
    {"mode", "int32", "(n)", "the mode number"},
    {"frequency", "float", "(n)", "the frequency of the mode, in cycles per unit time"},
    {"effectiveMass", "float", "(n, 6)",
     "the modal effective mass of the mode in the six directions of the rigid body motion, t1 t2 t3 r1 r2 r3"},
    {"fraction", "float", "(n, 6)",
     "the modal effective mass over the rigid body mass of each direction (NaN where the rigid body mass of the "
     "direction is zero)"},
    {"participation", "float", "(n, 6)", "the modal participation factors in the six directions"},
    {"rigidBodyMass", "float", "(6, 6)", "the rigid body mass matrix about the reference point"},
    {"referencePoint", "int64", "",
     "the grid point the rigid body motions are about (GRID= of MEFFMASS; 0: the origin of the basic system)", false,
     true},
});

// The integrated loads of the monitor points (MONPNT1, MONPNT2, MONPNT3, ...): a row a point, component, step and
// contribution.
constexpr auto MONITOR_POINT_LOADS = std::to_array<NamedArrayInfo>({
    {"subcase", "int32", "(n)", "the subcase"},
    {"step", "float", "(n)", "the frequency or the time of the value; NaN for a static or a trim subcase"},
    {"component", "int32", "(n)", "the degree of freedom of the value, 1 to 6: fx fy fz mx my mz along the axes of CD"},
    {"positionFrameId", "int32", "(n)", "the coordinate system of the point"},
    {"point", "float", "(n, 3)", "the point the moments are about, in CP"},
    {"displacementFrameId", "int32", "(n)", "the coordinate system of the components of the load"},
    {"value", "float or complex", "(n)", "the load (complex in a frequency response)"},
    {"name", "text", "(n)", "the name of the monitor point", true},
    {"label", "text", "(n)", "its label", true},
    {"card", "text", "(n)", "the card that defines it: MONPNT1, MONPNT2, MONPNT3, ...", true},
    {"pointClass", "text", "(n)",
     "the class of the point (COEF the whole vehicle, GNRL general, HM a hinge moment); empty for none", true},
    {"aeroComponent", "text", "(n)",
     "the AECOMP or AECOMPL a MONPNT1 integrates, the surface of a hinge moment; empty for none", true},
    {"side", "text", "(n)", "structural or aerodynamic: the loads on the structure or on the aerodynamic model", true},
    {"contribution", "text", "(n)",
     "what the value is: inertial, external, flexible (the flexible increment), gust, rigidAir, elasticRestrained, "
     "rigidApplied, restrainedApplied, resultant (a MONPNT3), total, ...",
     true},
    {"matrixRow", "int32", "(n)",
     "the row of the value in its load matrix, from 0 (the rows of a MONPNT3 matrix without a directory are named "
     "from the MONPNT3 cards of the deck by it)",
     false, true},
});

// The extensions of a companion package (no file layout): the loads of its sections and cuts and the contour data of
// its zones, a row labelled by the section, the cut or the zone (and the grid point), the subcase and the component
// of the model.
constexpr auto SECTION_LOADS = std::to_array<NamedArrayInfo>({
    {"subcase", "int32", "(n)", "the subcase"},
    {"step", "float", "(n)", "the step of the subcase (a time, a frequency, a mode); NaN for a static subcase"},
    {"componentId", "int32", "(n)", "the component of the model of the section; 0 for none"},
    {"loads", "float", "(n, 6)", "the force and the moment on the section: fx fy fz mx my mz, in the basic system"},
    {"point", "float", "(n, 3)", "the point the moment is about, in the basic system"},
    {"section", "text", "(n)", "the name of the section", true},
});

// The components of the sections of a companion package, one component a row: what a force and a moment cannot hold
// (the membrane forces, moments and shears of a shell section, the end forces of a bar), each named by its quantity.
constexpr auto SECTION_COMPONENTS = std::to_array<NamedArrayInfo>({
    {"subcase", "int32", "(n)", "the subcase"},
    {"step", "float", "(n)", "the step of the subcase (a time, a frequency, a mode); NaN for a static subcase"},
    {"componentId", "int32", "(n)", "the component of the model of the section; 0 for none"},
    {"value", "float", "(n)", "the value of the quantity"},
    {"section", "text", "(n)", "the name of the section", true},
    {"quantity", "text", "(n)", "what the value is (nx, mxy, qy, endA.axial, ...)", true},
});

constexpr auto CUT_LOADS = std::to_array<NamedArrayInfo>({
    {"nodeId", "int32", "(n)", "the grid point of the cut whose contribution the row holds; 0 for the whole cut"},
    {"subcase", "int32", "(n)", "the subcase"},
    {"step", "float", "(n)", "the step of the subcase; NaN for a static subcase"},
    {"componentId", "int32", "(n)", "the component of the model of the cut; 0 for none"},
    {"loads", "float", "(n, 6)", "the force and the moment through the cut: fx fy fz mx my mz, in the basic system"},
    {"point", "float", "(n, 3)", "the point the moment is about, in the basic system"},
    {"cut", "text", "(n)", "the name of the cut", true},
});

constexpr auto ZONE_CONTOURS = std::to_array<NamedArrayInfo>({
    {"nodeId", "int32", "(n)", "the grid point of the value"},
    {"subcase", "int32", "(n)", "the subcase"},
    {"step", "float", "(n)", "the step of the subcase; NaN for a static subcase"},
    {"componentId", "int32", "(n)", "the component of the model of the zone; 0 for none"},
    {"value", "float", "(n)", "the value of the contour at the grid point"},
    {"zone", "text", "(n)", "the name of the zone", true},
    {"quantity", "text", "(n)", "what the value is (vonMises, magnitude, ...)", true},
});

constexpr auto SUPERELEMENT_POINT_LIST = std::to_array<NamedArrayInfo>({
    {"superelement", "int32", "", "the superelement the list is of (0: the residual structure)"},
    {"nodeId", "int32", "(n)",
     "the points in the internal order of the superelement (a boundary point the solver creates in the residual "
     "structure: "
     "2 000 000 000 + n)"},
    {"code", "int32", "(n)",
     "the solver's code of each point: ID x 1000 for an interior point, 2 000 000 000 + n for boundary point n"},
    {"boundaryPoint", "int32", "(n)", "the boundary sequence number of each point, 0 for an interior point"},
});

constexpr auto KINDS = std::to_array<NamedKindInfo>({
    {"nodePointWeight",
     "the grid point weight generator: the rigid body mass matrix, the centre of gravity and the inertia of the model",
     WEIGHT},
    {"matrix",
     "a matrix: its stored terms as coordinate arrays (rows and columns from 0), with its shape, form and type",
     MATRIX},
    {"designVariables", "the design variables of a SOL 200 run: ID, label, bounds, move limit", DESIGN_VARIABLES},
    {"designHistory",
     "the history of the design iterations: objective, largest constraint, convergence, value of each variable",
     DESIGN_HISTORY},
    {"designVariableHistory",
     "the design variable history table: the objective, the approximate objective and the largest constraint of each "
     "cycle and the value of each variable",
     DESIGN_VARIABLE_HISTORY},
    {"responses", "the design responses (DRESP1) a SOL 200 run defines: type, label, subcase, item", RESPONSES},
    {"responseColumns", "the responses that are the columns of the sensitivity matrix (DRESP1 then DRESP2)",
     RESPONSE_COLUMNS},
    {"responseValues", "the value and the constraint bounds of each response at each design cycle", RESPONSE_VALUES},
    {"sensitivities", "the derivative of each response with respect to each design variable at each design cycle",
     SENSITIVITIES},
    {"aeroFlutterSummary",
     "the flutter summary of an aeroelastic analysis: for each mode and point, the Mach number, the density ratio, the "
     "velocity, the damping and the frequency",
     FLUTTER_SUMMARY},
    {"aeroTrimVariables", "the trim variables of a static aeroelastic analysis, their status and value",
     TRIM_VARIABLES},
    {"stabilityDerivatives",
     "the stability derivatives: the six force and moment coefficients of each trim variable, rigid, elastic and "
     "inertial",
     STABILITY_DERIVATIVES, true},
    {"hingeMomentDerivatives", "the hinge moment derivatives of a control surface with respect to each trim variable",
     HINGE_MOMENT_DERIVATIVES, true},
    {"controlSurfaceResults",
     "the position and the hinge moment of each control surface of a static aeroelastic analysis, with their limits",
     CONTROL_SURFACE_RESULTS},
    {"aeroPressures", "the pressure coefficient and the pressure on each aerodynamic element", AERO_PRESSURES},
    {"aeroForces", "the force and the moment on each aerodynamic grid point", AERO_FORCES, true},
    {"epsilon", "the error ratio EPSILON of the static solution of each subcase", EPSILON},
    {"externalWork", "the external work of the load of each subcase", EXTERNAL_WORK},
    {"envelope",
     "the extreme of each component over the load cases, with the load case that gives it, a row a grid point or "
     "place in an element and component",
     ENVELOPE},
    {"modalEffectiveMass",
     "the modal effective mass, its fraction of the rigid body mass and the participation factors of each mode "
     "(MEFFMASS)",
     MODAL_EFFECTIVE_MASS, true},
    {"monitorPointLoads", "the integrated loads of the monitor points, a row a point, component, step and contribution",
     MONITOR_POINT_LOADS, true},
    {"sectionLoads", "the loads of the sections a solver supplies, a row a section, subcase and step", SECTION_LOADS,
     true},
    {"sectionComponents",
     "the components of the sections a solver supplies, a row a section, quantity, subcase and step",
     SECTION_COMPONENTS},
    {"cutLoads", "the loads through the cuts a solver supplies, a row a cut (or grid point of a cut), subcase and step",
     CUT_LOADS, true},
    {"zoneContours", "the contour data of the zones a solver supplies, a row a zone, grid point, subcase and step",
     ZONE_CONTOURS},
    {"superelementPointList",
     "the points of a superelement in its internal order, each with the solver's code: the boundary points carry their "
     "boundary sequence number (the table of the residual structure maps each number to its residual point)",
     SUPERELEMENT_POINT_LIST},
});

} // namespace

std::span<const NamedKindInfo> allNamedKinds() noexcept
{
    return KINDS;
}

const NamedKindInfo* findNamedKind(std::string_view kind) noexcept
{
    for (const NamedKindInfo& info : KINDS)
        if (info.kind == kind)
            return &info;
    return nullptr;
}

} // namespace oor::results
