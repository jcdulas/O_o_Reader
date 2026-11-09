#include "ExplicitParamCatalogueData.hpp"

#include <initializer_list>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace oor::model
{

namespace
{

// The numbers a slot takes.
constexpr ExplicitParamRange anyNumber() noexcept
{
    return {};
}

constexpr ExplicitParamRange positive() noexcept
{
    ExplicitParamRange range;
    range.minimum = 0.0;
    range.excludesMinimum = true;
    return range;
}

constexpr ExplicitParamRange nonNegative() noexcept
{
    ExplicitParamRange range;
    range.minimum = 0.0;
    return range;
}

constexpr ExplicitParamRange between(double low, double high) noexcept
{
    ExplicitParamRange range;
    range.minimum = low;
    range.maximum = high;
    return range;
}

constexpr ExplicitParamRange positiveUpTo(double high) noexcept
{
    ExplicitParamRange range;
    range.minimum = 0.0;
    range.excludesMinimum = true;
    range.maximum = high;
    return range;
}

// What a blank value means.
constexpr ExplicitParamDefault noDefault() noexcept
{
    return {};
}

constexpr ExplicitParamDefault required() noexcept
{
    ExplicitParamDefault result;
    result.kind = ExplicitParamDefaultKind::Required;
    return result;
}

constexpr ExplicitParamDefault integerDefault(std::int32_t value) noexcept
{
    ExplicitParamDefault result;
    result.kind = ExplicitParamDefaultKind::Integer;
    result.integerValue = value;
    return result;
}

constexpr ExplicitParamDefault realDefault(double value) noexcept
{
    ExplicitParamDefault result;
    result.kind = ExplicitParamDefaultKind::Real;
    result.realValue = value;
    return result;
}

constexpr ExplicitParamDefault wordDefault(std::string_view word) noexcept
{
    ExplicitParamDefault result;
    result.kind = ExplicitParamDefaultKind::Word;
    result.word = word;
    return result;
}

constexpr ExplicitParamDefault sameAs(std::size_t slot) noexcept
{
    ExplicitParamDefault result;
    result.kind = ExplicitParamDefaultKind::SameAs;
    result.slot = slot;
    return result;
}

constexpr ExplicitParamDefault computedDefault() noexcept
{
    ExplicitParamDefault result;
    result.kind = ExplicitParamDefaultKind::Computed;
    return result;
}

// The slots, one function per kind of value.
ExplicitParamSlot integerSlot(const char* name, ExplicitParamRange range, ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::Integer;
    slot.range = range;
    slot.defaultValue = defaultValue;
    return slot;
}

ExplicitParamSlot realSlot(const char* name, ExplicitParamRange range, ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::Real;
    slot.range = range;
    slot.defaultValue = defaultValue;
    return slot;
}

ExplicitParamSlot wordSlot(const char* name, std::string_view words, ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::Word;
    slot.words = words;
    slot.defaultValue = defaultValue;
    return slot;
}

ExplicitParamSlot integerOrWordSlot(const char* name, ExplicitParamRange range, std::string_view words,
                                    ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::IntegerOrWord;
    slot.range = range;
    slot.words = words;
    slot.defaultValue = defaultValue;
    return slot;
}

ExplicitParamSlot tableSlot(const char* name, ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::Table;
    slot.defaultValue = defaultValue;
    return slot;
}

ExplicitParamSlot gridSlot(const char* name, ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::Grid;
    slot.defaultValue = defaultValue;
    return slot;
}

ExplicitParamSlot rigidBodySlot(const char* name, std::string_view words, ExplicitParamDefault defaultValue)
{
    ExplicitParamSlot slot;
    slot.name = name;
    slot.kind = ExplicitParamSlotKind::RigidBody;
    slot.words = words;
    slot.defaultValue = defaultValue;
    return slot;
}

// The forms and the entries.
ExplicitParamForm makeForm(std::string_view selector, std::vector<ExplicitParamSlot> slots)
{
    ExplicitParamForm form;
    form.selector = selector;
    form.slots = std::move(slots);
    return form;
}

// An entry with one layout of values.
ExplicitParamEntry simpleEntry(std::string_view name, std::vector<ExplicitParamSlot> slots)
{
    ExplicitParamEntry entry;
    entry.name = name;
    entry.forms.push_back(makeForm({}, std::move(slots)));
    return entry;
}

// An entry whose values are a few fixed ones and then a list of any length.
ExplicitParamEntry listEntry(std::string_view name, std::vector<ExplicitParamSlot> slots, ExplicitParamSlot tail,
                             std::size_t tailMinimum)
{
    ExplicitParamEntry entry;
    entry.name = name;
    entry.identityValues = -1; // one card per group of bodies: a deck gives as many as it needs
    entry.forms.push_back(makeForm({}, std::move(slots)));
    entry.forms.front().tail = std::move(tail);
    entry.forms.front().tailMinimum = tailMinimum;
    return entry;
}

// An entry whose layout depends on a word among its values.
ExplicitParamEntry entryWithForms(std::string_view name, int selectorIndex, const char* selectorName,
                                  int identityValues, std::vector<ExplicitParamForm> forms)
{
    ExplicitParamEntry entry;
    entry.name = name;
    entry.selectorIndex = selectorIndex;
    entry.selectorName = selectorName;
    entry.identityValues = identityValues;
    entry.forms = std::move(forms);
    return entry;
}

constexpr std::string_view YES_NO = "Yes|No";
constexpr std::string_view ON_OFF = "On|Off";

// The catalogue holds views of its words. The words built at run time are kept here, once each: a list never moves
// the texts it already holds.
std::string_view keptText(std::string text)
{
    static std::list<std::string> texts;
    return texts.emplace_back(std::move(text));
}

std::string_view valueWord(std::string_view name)
{
    return keptText(std::string(name));
}

// Words joined by '|', the form of ExplicitParamSlot::words.
std::string_view wordChoice(std::initializer_list<std::string_view> words)
{
    std::string joined;
    for (const std::string_view word : words)
        joined += (joined.empty() ? "" : "|") + std::string(word);
    return keptText(std::move(joined));
}

} // namespace

// The 80 entries of DYPARAM, in the order of the format: 78 names with one heading each, and the two headings of
// DYPARAM,SMP (the batch size and the CPU information of the shared-memory parallel solver), which are the two forms of
// one name. CONTACT is one heading with eleven options, each a form of its own.
//
// Conventions of the catalogue:
// - A value is "required" only where the format requires it. A value with a default in the format has it; a value
//   with neither may be blank, and a blank then stands for nothing the format states.
// - Where the default is a rule instead of a number (a fraction of a PARAM, the choice of the solver, a condition on
//   other settings), the slot has a computed default: the library does not evaluate it.
// - AMBIGUOUS comments mark the entries where the format is not consistent or does not say what the field holds.
const std::vector<ExplicitParamEntry>& explicitParamCatalogueEntries()
{
    // The words of the external explicit program and of the external explicit solver, which the bulk viscosity, the
    // contact defaults and the hourglass entries take as values.
    static const std::string_view EXTERNAL_PROGRAM = valueWord("ExternalExplicitProgram");
    static const std::string_view EXTERNAL_SOLVER = valueWord("ExternalExplicitSolver");

    static const std::vector<ExplicitParamEntry>
        ENTRIES =
            {
                // Output of the external occupant model.
                simpleEntry("OccupantAccelerationOutput",
                            {realSlot("outputInterval", positive(), realDefault(10.0e-3))}),
                simpleEntry("OccupantHistoryOutput", {wordSlot("historyOutput", YES_NO, wordDefault("Yes"))}),
                simpleEntry("OccupantTimeOutput", {realSlot("outputInterval", positive(), realDefault(1.0e-3))}),

                // Automatic coupling. FAIL has no default: it stays blank when the coupling surface has no failure.
                simpleEntry("AutoCoupling",
                            {wordSlot("activeState", ON_OFF, wordDefault("Off")),
                             wordSlot("cleanup", ON_OFF, wordDefault("Off")),
                             wordSlot("dummyFaces", ON_OFF, wordDefault("Off")),
                             realSlot("areaTolerance", nonNegative(), realDefault(0.0)),
                             wordSlot("outputKind", "Zero|Average|AverageDisplacement|MaximumUncoupled|Cover",
                                      wordDefault("Zero")),
                             wordSlot("failureHandling", "PorousKind|RemoveKind", noDefault())}),

                // Symmetric analyses. PHI is read for a rectangular mesh and ALIGN for an axial one; the format does
                // not refuse
                // the other, so both are accepted with either mesh type.
                simpleEntry("AxialSymmetry", {wordSlot("meshKind", "AxialMesh|RectRegion", required()),
                                              wordSlot("symmetryAxis", "XAxis|YAxis|ZAxis", required()),
                                              wordSlot("symmetryPlane", "PlaneXy|PlaneYz|PlaneZx", required()),
                                              realSlot("angle", anyNumber(), realDefault(0.0)),
                                              wordSlot("alignMesh", YES_NO, wordDefault("Yes")),
                                              realSlot("secondAngle", anyNumber(), realDefault(0.0))}),
                simpleEntry("AxialRemap", {realSlot("originX", anyNumber(), realDefault(0.0)),
                                           realSlot("originY", anyNumber(), realDefault(0.0)),
                                           realSlot("originZ", anyNumber(), realDefault(0.0)),
                                           realSlot("axisX", anyNumber(), realDefault(1.0)),
                                           realSlot("axisY", anyNumber(), realDefault(0.0)),
                                           realSlot("axisZ", anyNumber(), realDefault(0.0)),
                                           realSlot("remapRange", anyNumber(), realDefault(1.0e20))}),

                // Bulk viscosity.
                simpleEntry("LinearBulkViscosity", {realSlot("coefficient", nonNegative(), realDefault(0.0))}),
                simpleEntry("QuadraticBulkViscosity", {realSlot("coefficient", nonNegative(), realDefault(0.0))}),
                simpleEntry("BulkViscosityKind",
                            {wordSlot("formulation", wordChoice({EXTERNAL_PROGRAM, EXTERNAL_SOLVER}),
                                      wordDefault(EXTERNAL_PROGRAM))}),

                simpleEntry("FullRigidCoupling", {wordSlot("fullCoupling", YES_NO, wordDefault("Yes"))}),

                // The value is smaller than the PARAM FBLEND (0.66 unless given) and defaults to a third of it.
                // That is a rule between two parameters: it is not checked here.
                simpleEntry("ClumpFlowLimit", {realSlot("flowLimit", positive(), computedDefault())}),
                simpleEntry("ClumpEnergy", {wordSlot("energyRule", "Average|Sum", wordDefault("Average"))}),
                simpleEntry("PointMassOutput", {wordSlot("outputOn", YES_NO, wordDefault("No"))}),

                // Defaults of the contact definitions: an option and its values. Every option is a form. VERSION,
                // THICK, GAP,
                // DAMPING, DAMPFOR and EVIEW read their first value only; the option of the external explicit program
                // has no
                // value (the format marks it a required word but has no word for it, only the THICK and THICKOF it
                // implies).
                // AMBIGUOUS, FORCE: the format is not consistent on the default of NMCYC (10 or 100) and on the kind of
                // SCALE;
                // the detailed definition of each value is followed.
                entryWithForms("ContactDefaults", 0, "contactOption", 1,
                               {makeForm(
                                    "VersionKind", {wordSlot("contactOption", "VersionKind", required()),
                                                    wordSlot("contactVersion", "CodeV4|Belt|BeltElem1|DrawBeadContact",
                                                             required())}),
                                makeForm("ThicknessKind", {wordSlot("contactOption", "ThicknessKind", required()),
                                                           realSlot("thickness", positive(), required())}),
                                makeForm("GapKind", {wordSlot("contactOption", "GapKind", required()),
                                                     realSlot("gapDistance", positive(), required())}),
                                makeForm("LimitsKind", {wordSlot("contactOption", "LimitsKind", required()),
                                                        realSlot("minimumX", anyNumber(), realDefault(-1.0e20)),
                                                        realSlot("maximumX", anyNumber(), realDefault(1.0e20)),
                                                        realSlot("minimumY", anyNumber(), realDefault(-1.0e20)),
                                                        realSlot("maximumY", anyNumber(), realDefault(1.0e20)),
                                                        realSlot("minimumZ", anyNumber(), realDefault(-1.0e20)),
                                                        realSlot("maximumZ", anyNumber(), realDefault(1.0e20))}),
                                makeForm("DampingKind", {wordSlot("contactOption", "DampingKind", required()),
                                                         wordSlot("dampingOn", YES_NO, required())}),
                                makeForm("CouplingPorosity", {wordSlot("contactOption", "CouplingPorosity", required()),
                                                              wordSlot("porosityOn", YES_NO, wordDefault("No"))}),
                                makeForm("DampingForce", {wordSlot("contactOption", "DampingForce", required()),
                                                          wordSlot("dampingForceOn", YES_NO, required())}),
                                makeForm(EXTERNAL_PROGRAM, {wordSlot("contactOption", EXTERNAL_PROGRAM, required())}),
                                []
                                {
                                    ExplicitParamForm info = makeForm(
                                        "InformationKind", {wordSlot("contactOption", "InformationKind", required())});
                                    // The grid points whose contact state is printed, as many as given.
                                    info.tail = gridSlot("monitoredGrid", required());
                                    info.tailMinimum = 1;
                                    return info;
                                }(),
                                makeForm("ElemView", {wordSlot("contactOption", "ElemView", required()),
                                                      realSlot("edgeViewAngle", positive(), required())}),
                                makeForm("Force",
                                         {wordSlot("contactOption", "Force", required()),
                                          integerSlot("cycleCount", positive(), integerDefault(100)),
                                          realSlot("scaleFactor", positive(), realDefault(10.0)),
                                          wordSlot("forceKind", "Zero|MaxForceFactor", wordDefault("Zero"))})}),

                // The number of a TABLED1.
                simpleEntry("CouplingSubcycleMaxTime", {tableSlot("maxTimeTable", required())}),
                // The second word is NO-METAL, eight characters with the dash.
                simpleEntry("CouplingFriction", {wordSlot("frictionKind", "Metal|NonMetal", wordDefault("Metal"))}),
                // Each of the three is a positive real with no default: the friction law needs them, but the format
                // does not
                // call them required, so a blank is accepted.
                simpleEntry("CohesionOption", {realSlot("maxStress", positive(), noDefault()),
                                               realSlot("frictionCoefficient", positive(), noDefault()),
                                               realSlot("referenceVelocity", positive(), noDefault())}),

                // Distributed memory parallel options. AMBIGUOUS, VALUE4: a positive integer, but the only value the
                // format
                // lists is -1, so any integer is accepted; blank is "not used".
                simpleEntry("DistributedParallelOptions",
                            {integerSlot("setting1", between(1.0, 3.0), integerDefault(1)),
                             integerSlot("setting2", between(1.0, 2.0), integerDefault(1)),
                             integerSlot("setting3", nonNegative(), integerDefault(0)),
                             integerSlot("setting4", anyNumber(), noDefault())}),
                // AMBIGUOUS: the format gives INTERNAL both as the value and as the name of a YES/NO switch (default
                // NO), and
                // also uses ON for it. All four words are accepted.
                simpleEntry("DummySegments",
                            {wordSlot("internalSegments", "InternalKind|Yes|No|On", wordDefault("No"))}),
                // AMBIGUOUS: a number of elements, with only 0 (the 100 smallest time steps) and 1 (all of them)
                // listed; any
                // number of elements that is not negative is accepted.
                simpleEntry("SmallestTimeStepElemCount", {integerSlot("elemCount", nonNegative(), integerDefault(1))}),

                // Euler domain decomposition. NBY and NBZ are NBX when blank.
                simpleEntry("EulerCubeBlocks", {integerSlot("elemsPerCube", nonNegative(), integerDefault(0)),
                                                integerSlot("blockCountX", nonNegative(), integerDefault(0)),
                                                integerSlot("blockCountY", nonNegative(), sameAs(1)),
                                                integerSlot("blockCountZ", nonNegative(), sameAs(1))}),
                // PROCDIR USER needs NPX (a rule of the entry, checked with the values). NPY and NPZ are NPX when
                // blank.
                simpleEntry(
                    "EulerPartitioning",
                    {wordSlot("partitionDirection", "XAxis|YAxis|ZAxis|User|Simple|OrbitKind", wordDefault("XAxis")),
                     integerSlot("cubeCountX", nonNegative(), integerDefault(0)),
                     integerSlot("cubeCountY", nonNegative(), sameAs(1)),
                     integerSlot("cubeCountZ", nonNegative(), sameAs(1))}),
                simpleEntry("EulerTransport",
                            {wordSlot("transportScheme", "ImpulseLoad|Average", wordDefault("ImpulseLoad")),
                             wordSlot("failureHandling", "NoFailure|FailureCriterion", wordDefault("NoFailure"))}),
                // AMBIGUOUS, NSUBMAX: a positive integer with a default of 0, which the format also calls required. The
                // default is followed: a blank or a zero is accepted. DFVUMAX is needed with the coupling surface
                // subcycling of
                // COSUBMXT and must be blank otherwise (a rule between two entries, not checked); the value NONE of
                // LINKCS
                // needs it.
                simpleEntry("EulerSubcyclesMax",
                            {integerSlot("maxSubcycles", nonNegative(), integerDefault(0)),
                             realSlot("maxUncoveredFractionChange", positive(), noDefault()),
                             wordSlot("subcycleLink", "Both|CoupleSurface|Euler|None", wordDefault("Both"))}),
                simpleEntry("EulerSubcycles", {integerSlot("subcycleCount", positive(), integerDefault(1))}),

                simpleEntry("FailureTimeStep", {realSlot("timeStep", positive(), realDefault(1.0e-20))}),
                simpleEntry("FailureOutput", {wordSlot("outputOn", YES_NO, wordDefault("Yes"))}),
                // The description lists POLPACK and FACET and gives the default NO, which switches the method off.
                simpleEntry("FlowMethod", {wordSlot("method", "No|PolynomialPacking|FacetSurface", wordDefault("No"))}),
                simpleEntry("GeometryCheckOption", {wordSlot("checkOn", ON_OFF, wordDefault("Off"))}),

                // Fast coupling: seven options.
                simpleEntry(
                    "FastCoupling",
                    {wordSlot("inPlaneOption", "No|InPlaneMesh|NoOffset", wordDefault("InPlaneMesh")),
                     wordSlot("failureHandling", "NoFailure|FailureCriterion", wordDefault("NoFailure")),
                     wordSlot("eulerCheck", "CheckEulerElems|NoCheckEulerElems", wordDefault("NoCheckEulerElems")),
                     wordSlot("searchScope", "NearOnlyKind|All", wordDefault("All")),
                     wordSlot("faceOutput", "None|FaceEntities|PolynomialPackingShort|All", wordDefault("None")),
                     wordSlot("switch6", ON_OFF, wordDefault("On")), wordSlot("switch7", ON_OFF, wordDefault("On"))}),

                simpleEntry("HydrostaticBody", {realSlot("accelerationX", anyNumber(), realDefault(0.0)),
                                                realSlot("accelerationY", anyNumber(), realDefault(0.0)),
                                                realSlot("accelerationZ", anyNumber(), realDefault(0.0))}),

                // Hourglass damping coefficients, between 0 and 0.15. The default of each is 0.1, or the coefficient of
                // the
                // more general entry that is given (HGCOEFF for the others): a rule between entries, so the default is
                // computed.
                simpleEntry("HourglassMembraneCoefficient",
                            {realSlot("coefficient", between(0.0, 0.15), computedDefault())}),
                simpleEntry("HourglassCoefficient", {realSlot("coefficient", between(0.0, 0.15), computedDefault())}),
                simpleEntry("HourglassSolidCoefficient",
                            {realSlot("coefficient", between(0.0, 0.15), computedDefault())}),
                simpleEntry("HourglassTwistCoefficient",
                            {realSlot("coefficient", between(0.0, 0.15), computedDefault())}),
                simpleEntry("HourglassWarpCoefficient",
                            {realSlot("coefficient", between(0.0, 0.15), computedDefault())}),
                // Hourglass suppression methods: the default is FBV for shells and FBS for solids unless another entry
                // says so.
                simpleEntry("HourglassShell",
                            {wordSlot("method", wordChoice({"FlanaganBelytschkoViscous", EXTERNAL_PROGRAM}),
                                      computedDefault())}),
                simpleEntry("HourglassSolid",
                            {wordSlot("method", wordChoice({"FlanaganBelytschkoStiffness", EXTERNAL_PROGRAM}),
                                      computedDefault())}),
                simpleEntry("HourglassKind", {wordSlot("method",
                                                       wordChoice({"FlanaganBelytschkoStiffness",
                                                                   "FlanaganBelytschkoViscous", EXTERNAL_PROGRAM}),
                                                       computedDefault())}),

                // This entry can be given once.
                simpleEntry("HydrostaticInitialGravity", {realSlot("gravity", positive(), realDefault(9.80665))}),
                simpleEntry("HydroVolumeFailure", {wordSlot("failureOn", YES_NO, wordDefault("No"))}),

                // Initial metric method. The default of OPTION2 follows OPTION1 (OFF for FULL and REDUCED, ON for
                // ZERO).
                simpleEntry("InitialMetricMethod", {wordSlot("metricMethod", "Full|Reduced|Zero", wordDefault("Full")),
                                                    wordSlot("metricSwitch", "No|Off|Yes|On", computedDefault()),
                                                    realSlot("factor1", positive(), realDefault(1.0e-3)),
                                                    realSlot("factor2", positive(), realDefault(1.0e-3))}),
                // AMBIGUOUS: the format types the value both as a number and as a YES/NO flag; the flag is followed.
                simpleEntry("InfoObjects", {wordSlot("printOn", YES_NO, wordDefault("No"))}),
                simpleEntry("InitialStep", {realSlot("timeStep", positive(), required())}),

                // Lagrangian partitioning: IOPT 4 needs the vector, 5 and 6 the vector and the centre (checked with the
                // values).
                simpleEntry("LagrangePartitioning",
                            {integerSlot("partitionMethod", between(1.0, 7.0), integerDefault(1)),
                             realSlot("directionX", anyNumber(), realDefault(0.0)),
                             realSlot("directionY", anyNumber(), realDefault(0.0)),
                             realSlot("directionZ", anyNumber(), realDefault(0.0)),
                             realSlot("centreX", anyNumber(), realDefault(0.0)),
                             realSlot("centreY", anyNumber(), realDefault(0.0)),
                             realSlot("centreZ", anyNumber(), realDefault(0.0))}),
                simpleEntry("CubeLimit", {integerSlot("maxCubes", positive(), integerDefault(200))}),

                // Merges of rigid bodies. The names are RigidAssembly(id) and RigidMat(id),
                // as many
                // as the card holds. MATRMERG may instead be the single word AUTO; its first name is the new assembly,
                // an assembly
                // name.
                listEntry("RigidBodyMerge", {rigidBodySlot("newAssemblyName", "Automatic", required())},
                          rigidBodySlot("mergedBodyNames", {}, required()), 0),
                listEntry("RigidBodyMerge1", {rigidBodySlot("firstBodyName", {}, required())},
                          rigidBodySlot("mergedBodyNames", {}, required()), 1),

                simpleEntry("MaxStepSize", {realSlot("stepSize", positive(), realDefault(1.0e20))}),
                simpleEntry("MeshPlane", {integerSlot("plane", positive(), integerDefault(3))}),
                // The default is a tenth of INISTEP.
                simpleEntry("MinStepSize", {realSlot("stepSize", positive(), computedDefault())}),
                // The default is NO, and YES when the model has an INFLHB or an INFLGAS (a rule of the model).
                simpleEntry("GasMixing", {wordSlot("mixingOn", YES_NO, wordDefault("No"))}),
                simpleEntry("ZeroNodeVelocity", {wordSlot("zeroOn", YES_NO, wordDefault("No"))}),
                // AMBIGUOUS: the default is given as a rule, not a number: the old scheme is off unless asked for, so
                // 0.
                simpleEntry("OldLagrangeTime", {integerSlot("oldScheme", between(0.0, 1.0), integerDefault(0))}),
                simpleEntry("JonesWilkinsLeeDetonation",
                            {wordSlot("linkOption", "Link|NoLink", wordDefault("NoLink"))}),
                // The Roe solver: TYPE is ROE, OPTION is blank (second order in space) or NONE (first order).
                simpleEntry("LimiterKind", {wordSlot("solverKind", "RoeScheme", wordDefault("RoeScheme")),
                                            wordSlot("spatialOrder", "None", noDefault())}),
                // The first value is the word INFPAR, which the entry repeats.
                simpleEntry("ParallelOption", {wordSlot("settingWord", "InfoParameters", required()),
                                               wordSlot("printOn", ON_OFF, wordDefault("Off"))}),
                simpleEntry("PlasticCoverCut", {realSlot("cutoff", nonNegative(), realDefault(0.0))}),
                simpleEntry("MinPressureFailure", {wordSlot("failureOn", YES_NO, wordDefault("No"))}),
                simpleEntry("RigidBodyStarInfo",
                            {wordSlot("gridOutput", "NodeOutputOn|NodeOutputOff", wordDefault("NodeOutputOff"))}),
                // The default is a density of 1.E-5 times a characteristic density of the Euler material.
                simpleEntry("DensityCutoff", {realSlot("density", positive(), computedDefault())}),
                simpleEntry("RigidJointStiffness", {realSlot("stiffnessFactor", positive(), realDefault(1.0))}),
                simpleEntry("MassScaling", {realSlot("minTimeStep", positive(), required()),
                                            realSlot("maxPercent", positive(), required()),
                                            integerSlot("stepCount", positive(), required())}),

                // Shells.
                simpleEntry("ShellFormulation",
                            {wordSlot("formulation", "BelytschkoLinTsay|KeywordHoff", wordDefault("KeywordHoff"))}),
                simpleEntry("ShellElemSystem",
                            {wordSlot("systemKind", "Side21Kind|MidsideNodes", wordDefault("MidsideNodes"))}),
                simpleEntry("ShellPlasticity",
                            {wordSlot("plasticityMethod", "Radial|Vector|Iterative", wordDefault("Iterative"))}),
                simpleEntry("ShellStrainDefinition",
                            {wordSlot("strainDefinition", "Fiber|Elements", wordDefault("Fiber"))}),
                simpleEntry("ShellThicknessUpdate", {wordSlot("updateOn", YES_NO, wordDefault("Yes"))}),
                simpleEntry("SelectedElems", {wordSlot("selectionOn", YES_NO, wordDefault("Yes"))}),

                // Shared-memory parallel solver, two headings of one name:
                // DYPARAM,SMP,<entity>,BATCHSIZ,<value1>,CPULOOPS,<value2> and DYPARAM,SMP,<entity>,CPUINFO,<value1>.
                // The batch
                // size is a number or AUTOMAT (also spelled AUTOMATIC, which is too long for a
                // field). CPULOOPS and VALUE2 come together. AMBIGUOUS, CPUINFO: VALUE1 is marked
                // required and OFF is its default; blank is taken as OFF.
                entryWithForms(
                    "SharedMemoryParallel", 1, "settingKey", 2,
                    {makeForm(
                         "BatchSize",
                         {wordSlot("entityKind", "BeamEntities|ShellEntities|SolidEntities|ContactOutput", required()),
                          wordSlot("settingKey", "BatchSize", required()),
                          integerOrWordSlot("batchSize", positive(), "AutomaticSize", integerDefault(63)),
                          wordSlot("cpuLoopsWord", "ProcessorLoopCount", noDefault()),
                          integerSlot("cpuLoopCount", positive(), integerDefault(2))}),
                     makeForm("ProcessorInformation",
                              {wordSlot("entityKind",
                                        "BeamEntities|ShellEntities|SolidEntities|ContactOutput|"
                                        "MembraneKind|EulerType9|FluidStructureCouplingKind|All",
                                        required()),
                               wordSlot("settingKey", "ProcessorInformation", required()),
                               wordSlot("printCpuInfo", ON_OFF, wordDefault("Off"))})}),

                simpleEntry("SoundSpeedLimit", {realSlot("limit", positive(), realDefault(1.0e-3))}),
                simpleEntry("SphericalRemap", {realSlot("originX", anyNumber(), realDefault(0.0)),
                                               realSlot("originY", anyNumber(), realDefault(0.0)),
                                               realSlot("originZ", anyNumber(), realDefault(0.0)),
                                               realSlot("remapRange", positive(), realDefault(1.0e20))}),
                simpleEntry("SphericalSymmetry", {wordSlot("meshKind", "SphericalKind|RectRegion", required()),
                                                  wordSlot("radialAxis", "XAxis|YAxis|ZAxis", required()),
                                                  realSlot("angle", nonNegative(), realDefault(0.0))}),
                simpleEntry("StrainOutput", {wordSlot("outputOn", YES_NO, wordDefault("Yes"))}),
                // The value may not exceed 1.E-6 (the tolerance is 1.E-14 when the entry is absent).
                simpleEntry("ToleranceCheck", {realSlot("tolerance", positiveUpTo(1.0e-6), required())}),
                simpleEntry("VelocityDamping", {realSlot("dampingFactor", nonNegative(), realDefault(0.0))}),
                simpleEntry("MaxVelocity", {realSlot("maxVelocity", positive(), realDefault(1.0e10)),
                                            wordSlot("applyLimit", YES_NO, wordDefault("Yes"))}),
                simpleEntry("MaxVelocity1", {realSlot("maxVelocityTime", positive(), realDefault(1.0e20)),
                                             wordSlot("applyLimit", YES_NO, wordDefault("Yes")),
                                             realSlot("maxVelocityAdapted", positive(), realDefault(1.0e20))}),
                simpleEntry("Viscoplasticity", {integerSlot("viscoplasticOn", between(0.0, 1.0), integerDefault(0))}),
    };
    return ENTRIES;
}

} // namespace oor::model
