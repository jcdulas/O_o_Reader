#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <functional>
#include <optional>

namespace oor::model
{

// What a renumbering renumbers: one ID space, or the set IDs of one category of cards.
enum class RenumberSpace
{
    Node,
    Element,
    SurfaceElement, // CHBDYE, CHBDYG, CHBDYP
    Property,
    Material,
    ThermalMaterial, // MAT4 and MAT5
    Coordinate,
    Method,
    DynamicTable,
    MaterialTable,
    StressTable,
    HeatTable,
    HybridDampingRotorOptionsTable, // TABDMP1
    RandomTable,                    // TABRND1 and TABRNDG
    LoadCombinationSet,             // the set IDs of LOAD, FORCE, PLOAD4, ... (category Load)
    ConstraintSet,                  // the set IDs of SPC, SPC1, MPC, SPCADD (category Constraint)
    DynamicLoadSet,                 // the set IDs of TLOAD1, RLOAD1, DLOAD, DAREA, ... (category DynamicLoad)
    Set,                            // the IDs of SET1 and SET3 (category Set)
    FrequencySet,                   // the set IDs of FREQ, FREQ1 to FREQ5 (category Frequency)
    ComplexMethod,                  // the IDs of EIGC and EIGP (the real methods are the Method space)
    Delay,                          // the set IDs of DELAY
    Phase,                          // the set IDs of DPHASE
    TimeStep,             // the IDs of TSTEP and TSTEPNL (they number the same selection: one ID gives both a new one)
    Nonlinear,            // the IDs of NLPARM and NLPCI
    Random,               // the set IDs of RANDPS and RANDT1
    TransferFunction,     // the set IDs of TF
    InitialCondition,     // the set IDs of TIC
    HeatBoundaryProperty, // the IDs of PHBDY
    ConvectionProperty,   // the IDs of PCONV
    ForcedConvectionProperty, // the IDs of PCONVM
    RadiationMaterial,        // the IDs of RADM
    ViewFactorFaceFactor,     // the IDs of VIEW
    RadiationCavity,          // the cavity IDs of RADCAV, RADLST, RADMTX and VIEW3D
    ContactBody,              // BCBODY, BCBODY1, BCSURF, BCGRID: a body of a contact pair
    ContactSurface, // BSURF, BCPROP, BCBOX, ... and the rigid surfaces BCPATCH, BCBZIER, ...: what a body is made of
    ContactRigid,   // BCRIGID: the motion of a rigid contact body
    ContactBodyParameters,      // BCBDPRP: the parameters of a contact body
    ContactGeometricParameters, // BCONPRG: the geometric parameters of a contact pair
    ContactPhysicalParameters,  // BCONPRP: the physical parameters of a contact pair
    ContactPair,                // BCONECT and MDBCNCT: a contact pair
    ContactTable,               // BCTABL1, BCTABLE, BCTADD, BCTSET and MDBCTB1: a contact table
    TrimmingCurve,              // BCTRIM: the trimming curve of a NURBS surface
    SlidelineSegments,          // BLSEG: the line segments of a slideline contact region
    SlidelineContact,           // BCONP: a slideline contact region
    SlidelineFriction,          // BFRIC: the friction of a slideline contact region
    NonlinearStep,              // NLSTEP: the load or time stepping of a nonlinear analysis
    NonlinearHarmonic,          // NLHARM: the parameters of a nonlinear harmonic response
    NonlinearFrequency,         // NLFREQ, NLFREQ1 and NLFREQC: the forcing frequencies of a nonlinear harmonic response
    EquationOfState,            // EOSDEF, EOSGAM, EOSIG, ...: the equation of state a MATDEUL names
    ShearModel,                 // SHREL, SHRPOL, SHRUDS: the shear model a MATDEUL names
    YieldModel,                 // YLDHY, YLDJC, YLDVM, ...: the yield model a MATDEUL names
    FailureModel,               // FAILJC, FAILMPS, FAILUDS: the failure model a MATDEUL names
    SpallModel,                 // PMINC: the spall model a MATDEUL names
    EulerMesh,                  // MESH: an Euler mesh (a COUPLE, BARRIER or flow boundary names it)
    MeshBias,                   // BIAS: the mesh size variation a MESH names
    EulerBodyForce,             // EULFOR and EULFOR1: a body force on Euler elements
    EulerRegionSet,             // EULFREG: the set of Euler regions an EULFOR1 names
    EulerRegionShape,           // SURFINI, CYLINDR, SPHERE: a geometric shape that defines an Euler region
    Coupling,                   // COUPLE: an Euler-Lagrange coupling surface
    CouplingOptions,            // COUOPT: the set of coupling options a COUPLE names (OPTID)
    CohesiveFriction,           // COHFRIC: a cohesive friction model
    CohesiveFrictionSet,        // COUCOHF: the set of cohesive frictions a COUPLE names (COHFRID)
    CouplingFailure,            // COUP1FL: the surroundings of a failed coupling surface segment
    HydrostaticPreset,          // HYDSTAT: a hydrostatic preset of the Euler densities
    GasBag,                     // GBAG: a gas bag with a uniform pressure
    Leakage,                    // LEAKAGE: the set of porosity definitions a GBAG or COUPLE names (PORID)
    Porosity,    // PERMEAB, PERMGBG, PORFCPL, PORFGBG, PORFLOW, PORFLWT, PORHOLE, PORHYDS, PORUDS: the porosity model a
                 // LEAKAGE names
    InflatorSet, // ABINFL: the set of inflators a GBAG or COUPLE names (INFID)
    Inflator,    // INFLCG, INFLHB, INFLTNK, INFLTR: the inflator model an ABINFL names
    InflatorGas, // INFLGAS: a gas of an inflator or of an initial gas composition
    GasFractions,         // INFLFRC: the gas fractions of a hybrid inflator as a function of time
    InitialGas,           // INITGAS: the initial gas composition of a gas bag or a coupling surface
    DesignVariable,       // the IDs of DESVAR
    DesignEquation,       // the IDs of DEQATN
    DesignResponse,       // the IDs of DRESP1, DRESP2 and DRESP3 (one number space)
    DesignConstraintSet,  // the DCIDs of DCONSTR and DCONADD
    PropertyRelation,     // the IDs of DVPREL1 and DVPREL2
    MaterialRelation,     // the IDs of DVMREL1 and DVMREL2
    ConnectivityRelation, // the IDs of DVCREL1 and DVCREL2
    AeroElement,          // the EIDs of CAERO1 to CAERO5; the boxes (AeroBox references) follow their CAERO, so use a
                          // mapping that keeps the ranges apart (an offset)
    AeroProperty,         // the IDs of PAERO1 to PAERO5
    AeroList,             // the IDs of AELIST
    AeroFactor,           // the IDs of AEFACT
    AeroFlutterFactor,    // the IDs of FLFACT
    AeroControlSurface,   // the IDs of AESURF
    AeroTrimSet,          // the IDs of TRIM and TRIM2 (one selection: both get the same new ID)
    AeroFlutterSet,       // the IDs of FLUTTER
    AeroGustSet,          // the IDs of GUST
    DivergenceSet,        // the IDs of DIVERG
    ControlSchedule,      // the IDs of CSSCHD
    ControlVector,        // the IDs of UXVEC
    AeroNode,             // the IDs of AEGRID
    AeroScale,            // the IDs of AESCALE
    Superelement,    // the superelement numbers: the SEID of SESET, SEELT, SEBULK, CSUPEXT, CSUPER, every reference to
                     // one, the SEID of GRID and GRDSET, the Case Control selections and the BEGIN SUPER=n lines of
                     // the parts (one number space for the whole deck: the residual structure and its parts)
    MassCombination, // the IDs of MASSSET
    AeroSpline,      // the IDs of SPLINE1 to SPLINE7, SPLINEX, SPBLND1, SPBLND2 and SPLINRB (the cards of one category,
                     // IDs unique per card type: two types with one ID are one selection and get one new ID)
    HybridDampingRotorOptionsSpecification, // the IDs of DAMPING
    HybridDamping,                          // the IDs of HYBDAMP
    ElementList,                            // the IDs of ELIST
    ModuleMove,                             // the MVIDs of MDTRAN, MDROT1, MDROT2, MDMIR1 and MDMIR2
    MultiDimensionalTable,                  // the IDs of TABL3D0, TABL3D1, TABL3D2 and TABLE3D
    FatigueTable,                           // the IDs of TABLFTG
    ImperfectionCase,                       // the IDs of IMPCASE
    ImperfectionGeometry,                   // the IDs of IMPGEOM
    GeometricBoundary,                      // the IDs of GMBNDC
    AdaptivityCriterion,                    // the IDs of HADACRI
    AdaptivitySet,                          // the IDs of HADAPTL
    Rotor,                                  // the ROTORIDs of ROTORG, ROTOR, ROTORAX and ROTORSE
    DiscreteValueSet,                       // the IDs of DDVAL
    UserDefinedName,                        // the UDIDs of UDNAME
    FatigueProperty,                        // the IDs of PFTG
    FatigueLoad,                            // the IDs of FTGLOAD
    FatigueEvent,                           // the IDs of FTGEVNT and FTGSEQ
    EulerInitialization,                    // the IDs of TICEUL1
    EulerInitialRegions,                    // the TSIDs of TICREG
    InitialValueSet,                        // the IDs of TICVAL
    HeatLossSet,                            // the HTRIDs of HEATLOS
    HeatTransferModel,                      // the IDs of HTRCONV and HTRRAD
    CosimulationGroup,                      // the IDs of COSMGRP
    CosimulationInput,                      // the IDs of COSMINP
    CosimulationOutput,                     // the IDs of COSMOUT
    CosimulationService,                    // the IDs of COSMSRV
    WettedElement,                          // the WEIDs of WETELME and WETELMG
    WettedSurface,                          // the WSIDs of WETSURF
    BrakeProperty,                          // the IDs of BRKPROP
    BrakeMotion,                            // the IDs of MOTION
    BrakeSqueal,                            // the IDs of BSQUEAL and BRKSYS
    FrfComponent,                           // the COMPIDs of FRFCOMP
    BushingForceSet,                        // the LIDs of LBSH3DG
    LoadStepControl,                        // the IDs of TABSCTL
    TemperatureGroup,                       // the IDs of TMPSET
    MonteCarloSet,                          // the SIDs of MONCARL
    DeactivatedElementSet,                  // the IDs of DEACTEL
    TransferPathSet,                        // the SETIDs of TPADEFN
    IterativeSolver,                        // the SIDs of ITER
    MetadataGroup                           // the ABSTIDs of ABSTRCT
};

// The new ID of an old one; nothing leaves it unchanged.
using IdMapping = std::function<std::optional<base::EntityId>(base::EntityId)>;
// Makes the mapping for one model (the residual structure or a part): the mapping of a renumbering "from 1000, in file
// order" depends on the model it is applied to.
using MappingFactory = std::function<IdMapping(const Model&)>;

// Renumbering of one ID space, with every reference to it rewritten.
struct Renumbering
{
    // Gives every ID of the space a new one and rewrites the references to them, in the model and (with
    // `includeParts`) in each of its parts, which have ID spaces of their own, each with the mapping of its own IDs.
    // Atomic: when a new ID is not valid, or two IDs would become the same (or one becomes an ID that stays), nothing
    // is changed and DuplicateIdError or UsageError is thrown. Returns the number of cards whose ID changed. A THRU
    // range keeps its end points renumbered (it stays correct for an offset or any increasing mapping).
    //
    // A part is renumbered on its own too (`apply` on the part): what the residual structure names in it (the grid
    // points of a SECONCT, SEBNDRY or SEEXCLD, the design variables of a SEDLINK:
    // ReferenceVisitor::visitInSuperelement) follows, and nothing else changes. The residual structure alone leaves the
    // parts as they are, and so does a part leave the others. The superelement numbers are the exception: they belong
    // to the whole deck, so that space is renumbered on the residual structure (never on a part) and takes the parts,
    // whatever `includeParts` says.
    static std::size_t apply(Model& model, RenumberSpace space, const MappingFactory& factory,
                             bool includeParts = false);

    // The mapping that adds `offset` to every ID.
    [[nodiscard]] static MappingFactory offset(base::EntityId offset);
    // The mapping that gives the IDs of the space, in file order, `start`, `start + step`, ...
    [[nodiscard]] static MappingFactory consecutive(RenumberSpace space, base::EntityId start, base::EntityId step);
    // A mapping given as a function (for a dictionary or a callable).
    [[nodiscard]] static MappingFactory fromFunction(IdMapping mapping);
};

} // namespace oor::model
