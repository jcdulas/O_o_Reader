#pragma once

#include "oor/base/ids/IdSpace.hpp"

#include <cstdint>
#include <string_view>

namespace oor::model
{

// The containers of the model: which group of the object model a card type belongs to.
enum class CardCategory : std::uint8_t
{
    Unknown, // cards kept as text
    Node,    // GRID: the grid points (model.nodes)
    Point,   // SPOINT, EPOINT, POINT, GRIDB: the other points, which share the node ID space (model.points)
    Coordinate,
    Element,
    Property,
    Material,
    Load,
    Constraint,
    DynamicLoad,
    Method,
    Frequency,
    ComplexMethod,    // EIGC and EIGP: the complex eigenvalue extraction (CMETHOD)
    Delay,            // DELAY: the time delays of the dynamic loads
    PhaseLead,        // DPHASE: the phase leads of the dynamic loads
    TimeStep,         // TSTEP and TSTEPNL: the time steps of a transient analysis (TSTEP)
    NonlinearStatic,  // NLPARM and NLPCI: the parameters of a nonlinear static analysis (NLPARM)
    Random,           // RANDPS and RANDT1: the random analysis sets (RANDOM)
    TransferFunction, // TF: the transfer functions (TFL)
    InitialCondition, // TIC: the transient initial conditions (IC)
    Table,
    Set,
    Parameter,
    Matrix,                   // DMIG, DMIJ, DMIJI, DMIK, DMIAX and DMI: the direct matrix input (model.matrices)
    MonitorPoint,             // MONPNT1, MONPNT2, MONPNT3 and MONDSP1: the monitor points (model.monitor_points)
    ThermalMaterial,          // MAT4 and MAT5: the heat transfer materials (model.thermal_materials)
    HeatBoundaryProperty,     // PHBDY: the property of a CHBDYP surface element (its own ID per PHBDY)
    ConvectionProperty,       // PCONV: the free convection property that a CONV names
    ForcedConvectionProperty, // PCONVM: the forced convection property that a CONVM names
    RadiationMaterial,        // RADM: the radiation properties a surface element names (RADMIDF, RADMIDB)
    ViewFactorFaceFactor,     // VIEW: the view factor definition a surface element names (IVIEWF, IVIEWB)
    RadiationCavity,          // RADCAV, RADLST, RADMTX, VIEW3D: the cards of one radiation cavity (ICAVITY)
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
    DesignVariable,       // DESVAR: a design variable (the DVIDi of every relation and of DLINK name one)
    DesignEquation,       // DEQATN: an equation (the EQID of DVPREL2, DVMREL2, DVCREL2 and DRESP2, and of the
                          // PBUSH1D and CORD3G that take their data from an equation)
    DesignResponse,       // DRESP1, DRESP2 and DRESP3: a response; the three share one number space
    DesignConstraint,     // DCONSTR and DCONADD: a set of design constraints (DESSUB and DESGLB name one)
    PropertyRelation,     // DVPREL1 and DVPREL2: the relation of design variables to a property field
    MaterialRelation,     // DVMREL1 and DVMREL2: the relation of design variables to a material field
    ConnectivityRelation, // DVCREL1 and DVCREL2: the relation of design variables to a connectivity field
    AeroElement,          // CAERO1 to CAERO5: the aerodynamic macro elements (model.caeros)
    AeroProperty,         // PAERO1 to PAERO5: the properties of the macro elements (model.paeros)
    AeroList,           // AELIST: a list of aerodynamic boxes or grid points (a control surface, a spline, a component)
    AeroFactor,         // AEFACT: a list of numbers (the divisions of a CAERO, the data of a PAERO, a schedule)
    AeroFlutterFactor,  // FLFACT: the densities, Mach numbers, frequencies or velocities a FLUTTER names
    AeroSpline,         // SPLINE1 to SPLINE7 and SPLINEX: the interpolation of the structure to the aerodynamics
    AeroControlSurface, // AESURF: a control surface (a CSSCHD names it; a TRIM names its label)
    AeroTrimVariable,   // AESTAT and AEPARM: the rigid body motions and the general controllers of a trim
    AeroTrimSet,        // TRIM and TRIM2: a trim case (Case Control TRIM)
    AeroFlutterSet,     // FLUTTER: a flutter analysis (Case Control FMETHOD)
    AeroGustSet,        // GUST: a gust (Case Control GUST)
    DivergenceSet,      // DIVERG: a divergence analysis (Case Control DIVERG)
    ControlSchedule,    // CSSCHD: the schedule of a control surface (Case Control CSSCHD)
    ControlVector,      // UXVEC: a vector of control parameters (Case Control AEUXREF, AEDW, AEFORCE, AEPRESS)
    AeroNode,           // AEGRID: an aerodynamic grid point (AEQUAD4 and AETRIA3 name them)
    AeroScale,          // AESCALE: the reference lengths an AEGRID names
    AeroComponent,      // AECOMP and AECOMPL: a component of an integrated load monitor point, keyed by its name
    Aero,               // AERO, AEROS and the other aeroelastic cards no other card names by number
    Superelement,       // SESET, SEBULK, SECONCT, CSUPER, ...: the cards of the superelement family
                  // (model.superelement_cards); those that define a superelement number (SESET, SEELT, SEBULK, CSUPEXT,
                  // CSUPER) are IdSpace::Group
    MassCombination,                        // MASSSET: a linear combination of mass cases (Case Control MASSSET)
    HybridDampingRotorOptionsSpecification, // DAMPING: parameter and hybrid damping (Case Control RSDAMP and SEDAMP)
    HybridDamping,                          // HYBDAMP: the hybrid modal damping a DAMPING or a ROTHYBD names
    ElementList, // ELIST: a list of CQUAD4 and CTRIA3 elements for virtual fluid mass (MFLUID)
    ModuleMove,  // MDTRAN, MDROT1, MDROT2, MDMIR1, MDMIR2: a reposition of a module (an MDMOVE names them by MVID)
    MultiDimensionalTable, // TABL3D0, TABL3D1, TABL3D2 and TABLE3D: tables of several variables
    FatigueTable,          // TABLFTG: the cyclic loading table of a fatigue load (FTGLOAD)
    ImperfectionCase,      // IMPCASE: a collection of imperfections (Case Control IMPERFECT)
    ImperfectionGeometry,  // IMPGEOM: an imperfection that an IMPCASE names
    GeometricBoundary,     // GMBNDC: a boundary curve that a CINTC names
    AdaptivityCriterion,   // HADACRI: a mesh refinement criterion that a HADAPTL names
    AdaptivitySet,         // HADAPTL: the control of the local adaptive mesh refinement (Case Control HADAPT)
    Rotor,                 // ROTORG, ROTOR, ROTORAX and ROTORSE: a rotor (the ROTORID of the rotor dynamics cards)
    DiscreteValueSet,      // DDVAL: a list of discrete values (a DESVAR, TOMVAR, CAMPBLL or RSPINR names one)
    UserDefinedName,       // UDNAME: a file name that FTGLOAD, MATDIGI, HDF5DA and the load history file table name
    FatigueProperty,       // PFTG: the fatigue properties a FTGDEF names
    FatigueLoad,           // FTGLOAD: a cyclic loading variation a FTGEVNT names
    FatigueEvent,          // FTGEVNT and FTGSEQ: a loading event or a load sequence (they share one ID space)
    EulerInitialization,   // TICEUL1: the initial conditions of Euler regions (a PEULER1 names one)
    EulerInitialRegions,   // TICREG: a group of initial regions (a TICEUL1 names the group)
    InitialValueSet,       // TICVAL: the initial values of an Euler region (a TICREG names one)
    HeatLossSet,           // HEATLOS: a set of heat loss definitions (a COUPLE or a GBAG names one by HTRID)
    HeatTransferModel,     // HTRCONV and HTRRAD: the convection or radiation model a HEATLOS names
    CosimulationGroup,     // COSMGRP: the coupled area of a co-simulation (a COSMSRV names it)
    CosimulationInput,     // COSMINP: the input quantities of a co-simulation (a COSMSRV names them)
    CosimulationOutput,    // COSMOUT: the output quantities of a co-simulation (a COSMSRV names them)
    CosimulationService,   // COSMSRV: a co-simulation service (a COSMSEL names it)
    WettedElement,         // WETELME and WETELMG: a wetted element of a fluid-structure service (one ID space)
    WettedSurface,         // WETSURF: a wetted surface of a fluid-structure service (a WETLOAD names it)
    BrakeProperty,         // BRKPROP: the property of a brake system (a BRKSYS names it)
    BrakeMotion,           // MOTION: the motion direction of a brake system (a BRKSYS names it)
    BrakeSqueal,           // BSQUEAL, BRKSYS and MDBKSYS: a brake squeal analysis (Case Control BSQUEAL)
    FrfComponent,     // FRFCOMP: a component of an FRF based assembly (the other FRF cards name it by ID or by name)
    BushingForceSet,  // LBSH3DG: the force tables of a rotor bushing (an NLBSH3D names them)
    LoadStepControl,  // TABSCTL: the user criteria of load stepping (an NLSTEP names it)
    TemperatureGroup, // TMPSET: a group of grid points with one temperature history (a TTEMP names it)
    MonteCarloSet,    // MONCARL: the parameters of a Monte-Carlo simulation (Case Control MONCARL)
    DeactivatedElementSet, // DEACTEL: the elements that do not take part in an analysis (Case Control DEACTEL)
    TransferPathSet,       // TPADEFN: the set-up of a transfer path analysis (Case Control TPASET)
    IterativeSolver,       // ITER: the options of the iterative solver (Case Control SMETHOD)
    MetadataGroup,         // ABSTRCT: a meta data group (the ABSNMVB, ABSNMVI, ABSNMVR and ABSNMVS cards name it)
    Other
};

// What the model needs to know about a card type without looking at its fields.
struct CardTypeInfo
{
    std::string_view name;
    CardCategory category = CardCategory::Other;
    // The ID space the card's ID lives in (IdSpace::Group for a set ID shared by several cards, None for a card
    // without an ID).
    base::IdSpace idSpace = base::IdSpace::None;
    // True when a deck may define the same ID twice and the repeat is the same object, not an error (SPOINT and EPOINT:
    // the format reference allows repeated scalar point IDs). The reader keeps one row for the ID.
    bool repeatsAllowed = false;
};

} // namespace oor::model
