#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string_view>

namespace oor::model
{

// What a reference field points to. Most are ID spaces; the others are sets that several cards share.
enum class ReferenceTarget
{
    Node,        // a GRID, or any other point of the node ID space (a scalar point is one)
    NodeInRange, // a node ID in a THRU range: renumbered like a node, but allowed not to exist (the solver skips the
                 // gaps)
    ScalarOrGridPoint, // a field that takes a grid point or a scalar point (CELAS1 G1, CBEAM SA): renumbered like a
                       // node; it may name a scalar point that no SPOINT defines (the format reference lets a scalar
                       // element define its scalar points by naming them), so it is allowed not to exist
    Element,
    ElementInRange, // an element ID in a THRU range (PLOAD2, TEMPP1, TEMPRB): renumbered like an element, but allowed
                    // not to exist (the solver skips the gaps)
    SurfaceElement, // a CHBDYE, CHBDYG or CHBDYP: the heat transfer surface element (CONV, QBDY1, RADBC, ...)
    SurfaceElementInRange, // a surface element in a THRU list: renumbered like one, but allowed not to exist
    Property,
    PropertyInRange, // a property ID of a non-structural mass card (NSM, NSM1, NSML, NSML1): renumbered like a
                     // property, but allowed not to exist (undefined property IDs are ignored)
    Material, // a MAT1, MAT2, ...; a field that names the material of a property may also name a MAT4 or MAT5 (below)
    ThermalMaterial, // a MAT4 or MAT5 (the MID of a MATT4, MATT5, PCONV, PCONVM)
    Coordinate,
    DynamicTable,
    MaterialTable,
    StressTable,                    // a TABLES1 or TABLEST (MATS1, CREEP, MATHP, ...)
    HeatTable,                      // a TABLEH1 or TABLEHT
    HybridDampingRotorOptionsTable, // a TABDMP1 (SDAMPING also takes a TABLEDi: it is looked for in the dynamic table
                                    // space too)
    RandomTable,                    // a TABRND1 or TABRNDG (RANDPS)
    Method,
    LoadCombinationSet,  // a LOAD/FORCE/... set ID
    ConstraintSet,       // an SPC/SPCADD/MPC set ID
    DynamicLoadSet,      // the set ID of an RLOAD1/TLOAD1/DAREA/... card (a DLOAD term)
    ExcitationSet,       // the set a dynamic load excites with: a DAREA set or a static load set (EXCITEID)
    ExcitationDefined,   // the EXCITEID that an LSEQ card defines: not reported (it is the definition), renumbered like
                         // ExcitationSet, and found by an EXCITEID that names it
    FrequencySet,        // the set ID of FREQ, FREQ1 to FREQ5
    ComplexMethod,       // the ID of an EIGC (and of the EIGP cards that go with it): CMETHOD
    DelaySet,            // the set ID of a DELAY (the DELAY field of a dynamic load)
    PhaseSet,            // the set ID of a DPHASE
    TimeStepSet,         // the ID of a TSTEP or TSTEPNL
    NonlinearSet,        // the ID of an NLPARM (and of its NLPCI)
    RandomSet,           // the set ID of RANDPS and RANDT1 cards
    TransferFunctionSet, // the set ID of TF cards
    InitialConditionSet, // the set ID of TIC cards (IC also names a set of temperatures: a TEMP or TEMPD set)
    Set,                 // a SET1/SET3 ID
    HeatBoundaryProperty,     // the PID of a PHBDY (CHBDYP)
    ConvectionProperty,       // the PCONID of a PCONV (CONV)
    ForcedConvectionProperty, // the PCONID of a PCONVM (CONVM)
    RadiationMaterial,        // the RADMID of a RADM (the RADMIDF and RADMIDB of the surface elements)
    ViewFactorFaceFactor,     // the IVIEW of a VIEW (the IVIEWF and IVIEWB of the surface elements)
    RadiationCavity,          // the ICAVITY of a RADCAV, RADLST, RADMTX or VIEW3D
    ContactBody,              // BCBODY, BCBODY1, BCSURF, BCGRID: a body of a contact pair
    ContactSurface, // BSURF, BCPROP, BCBOX, ... and the rigid surfaces BCPATCH, BCBZIER, ...: what a body is made of
    ContactRigid,   // BCRIGID: the motion of a rigid contact body
    ContactBodyParameters,      // BCBDPRP: the parameters of a contact body
    ContactGeometricParameters, // BCONPRG: the geometric parameters of a contact pair
    ContactPhysicalParameters,  // BCONPRP: the physical parameters of a contact pair
    ContactPair,                // BCONECT and MDBCNCT: a contact pair
    ContactTable,               // BCTABL1, BCTABLE, BCTADD, BCTSET and MDBCTB1: a contact table
    ContactSelection,           // what Case Control BCONTACT=n selects: a BCTABL1 or BCTABLE, or directly a BCONECT
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
    DesignVariable,       // the ID of a DESVAR (DVIDi, DDVID, IDVi)
    DesignEquation,       // the EQID of a DEQATN
    DesignResponse,       // a DRESP1, DRESP2 or DRESP3 (RID of a DCONSTR, NRk and NRRk of a DRESP2, DESOBJ)
    DesignConstraintSet,  // the DCID of a DCONSTR or a DCONADD (DCi of a DCONADD, DESSUB, DESGLB)
    PropertyRelation,     // the ID of a DVPREL1 or a DVPREL2 (DPIPi, DPI2Pi of a DRESP2 or DRESP3)
    MaterialRelation,     // the ID of a DVMREL1 or a DVMREL2 (DMIMi, DMI2Mi)
    ConnectivityRelation, // the ID of a DVCREL1 or a DVCREL2 (DCICi, DCI2Ci)
    AeroElement,          // a CAERO1 to CAERO5 (the CAERO of a SPLINE, a B of a PAERO1, a CAERO list of an AECOMP)
    AeroPanelOrComponent, // the CAERO of a SPLINE4: a CAERO1 to CAERO5, or the component (CMPID) of the AEQUAD4 and
                          // AETRIA3 of an aerodynamic mesh (format reference SPLINE4: "aerodynamic component");
                          // renumbered like a CAERO where a CAERO has the ID
    AeroBox, // an aerodynamic box: an ID in the range a CAERO takes from its EID on (BOX1, ID1, BOXID of a SPLINE, the
             // items of an AELIST). It may name nothing (an AELIST of a Mach box or an aerodynamic mesh holds other
             // numbers) and is renumbered with the CAERO that owns it
    AeroProperty,       // a PAERO1 to PAERO5 (the PID of a CAERO)
    AeroList,           // an AELIST (an AESURF, an AECOMP, a SPLINE4 or a SPLINE5)
    AeroFactor,         // an AEFACT (the divisions of a CAERO, the arrays of a PAERO, the data of a CSSCHD)
    AeroFlutterFactor,  // an FLFACT (the DENS, MACH and RFREQ or VEL of a FLUTTER)
    AeroControlSurface, // an AESURF (the AESID of a CSSCHD)
    AeroTrimSet,        // a TRIM or TRIM2 (Case Control TRIM)
    AeroFlutterSet,     // a FLUTTER (Case Control FMETHOD)
    AeroGustSet,        // a GUST (Case Control GUST)
    DivergenceSet,      // a DIVERG (Case Control DIVERG)
    ControlSchedule,    // a CSSCHD (Case Control CSSCHD)
    ControlVector,      // a UXVEC (Case Control AEUXREF, the UXID of an AEDW, AEFORCE or AEPRESS)
    AeroNode,           // an AEGRID (the points of an AEQUAD4 or an AETRIA3)
    AeroScale,          // an AESCALE (the ASID of an AEGRID)
    Superelement,  // a superelement number: a part (BEGIN SUPER=n), a list superelement (SESET, SEELT, SEBULK, CSUPEXT,
                   // CSUPER, or the SEID of a GRID) or 0, the residual structure (the SEID of nearly every card of
                   // the family, and of the Case Control SUPER, SEALL, SEDR, ...)
    MassIncrement, // the number of a mass increment: BEGIN MASSID=n, or the MASSID of BEGIN SUPER=n MASSID=m (the IDi
                   // of a MASSSET); 0 is the base mass case
    MassCombination, // a MASSSET (Case Control MASSSET)
    AeroSpline, // a SPLINE1 to SPLINE7, SPLINEX, SPBLND1, SPBLND2 or SPLINRB (SID1 and SID2 of a SPBLND or SPRELAX)
    HybridDampingRotorOptionsSpecification, // a DAMPING (Case Control RSDAMP and SEDAMP)
    HybridDamping,                          // a HYBDAMP (the HYBRID of a DAMPING, the HYBDAMPi of a ROTHYBD)
    ElementList,                            // an ELIST (ELIST1 and ELIST2 of an MFLUID)
    ModuleMove,                             // an MDTRAN, MDROT1, MDROT2, MDMIR1 or MDMIR2 (the MVIDi of an MDMOVE)
    MultiDimensionalTable, // a TABL3Di or TABLE3D (the SOL 400 materials, a CORD3G); a TABLEMi field falls back to it
    StressOrMultiDimensionalTable, // the FID of a MATEP: a TABLES1 or TABLEST, or a TABL3Di (format reference MATEP:
                                   // "TABLES1 or TABL3D"); looked up and renumbered like StressTable where a stress
                                   // table has the ID
    FatigueTable,                  // a TABLFTG (the TID of an FTGLOAD)
    ImperfectionCase,              // an IMPCASE (Case Control IMPERFECT)
    ImperfectionGeometry,          // an IMPGEOM (the IMPFIDi of an IMPCASE)
    GeometricBoundary,             // a GMBNDC (the BIDs of the LIST of a CINTC)
    AdaptivityCriterion,           // a HADACRI (the CRITID of a HADAPTL)
    AdaptivitySet,                 // a HADAPTL (Case Control HADAPT)
    Rotor, // a ROTORG, ROTOR, ROTORAX or ROTORSE (the ROTORID of an RSPINR, RSPINT, REFROT, REFRTSE, ROTBENT and
           // ROTHYBD)
    DiscreteValueSet,    // a DDVAL (the DDVAL of a DESVAR, the DSVID of a TOMVAR, the DDVALID of a CAMPBLL)
    UserDefinedName,     // a UDNAME (the UID of the load history file table, the UDID of an HDF5DA, an FTGLOAD file)
    FatigueProperty,     // a PFTG (the PFTGID of a FTGDEF)
    FatigueLoad,         // an FTGLOAD (the FLOADi of an FTGEVNT)
    FatigueEvent,        // an FTGEVNT or an FTGSEQ (the FIDi of an FTGSEQ)
    EulerInitialization, // a TICEUL1 (the SID of a PEULER1)
    EulerInitialRegions, // the group of TICREG cards that share a TSID (the TSID of a TICEUL1)
    InitialValueSet,     // a TICVAL (the second TSID of a TICREG)
    HeatLossSet,         // the HEATLOS cards that share an HTRID (the HTRID of a COUPLE and of a GBAG)
    HeatTransferModel,   // an HTRCONV or an HTRRAD (the HTRTYPID of a HEATLOS)
    CosimulationGroup,   // a COSMGRP (the GRPID1 and GRPID2 of a COSMSRV)
    CosimulationInput,   // a COSMINP (the PHYINP of a COSMSRV)
    CosimulationOutput,  // a COSMOUT (the PHYOUT of a COSMSRV)
    CosimulationService, // a COSMSRV (the CSRVIDi of a COSMSEL)
    WettedElement,       // a WETELME or a WETELMG (the WEIDi of a WETSURF)
    WettedSurface,       // a WETSURF (the WSID of a WETLOAD)
    BrakeProperty,       // a BRKPROP (the BProp_ID of a BRKSYS)
    BrakeMotion,         // a MOTION (the motion IDs of a BRKSYS)
    BrakeSqueal,         // a BSQUEAL, a BRKSYS or an MDBKSYS (Case Control BSQUEAL)
    FrfComponent,     // an FRFCOMP (the COMPID of an FRFCONN, FRFFLEX, FRFRELS, FRFSPC1, FBADLAY, FBALOAD and FBAPHAS)
    BushingForceSet,  // an LBSH3DG (the LID of an NLBSH3D)
    LoadStepControl,  // a TABSCTL (the table of an NLSTEP)
    TemperatureGroup, // a TMPSET (the GROUP_ID of a TTEMP)
    MonteCarloSet,    // a MONCARL (Case Control MONCARL)
    DeactivatedElementSet, // a DEACTEL (Case Control DEACTEL)
    TransferPathSet,       // a TPADEFN (Case Control TPASET)
    IterativeSolver,       // an ITER (Case Control SMETHOD)
    MetadataGroup,         // an ABSTRCT (the ABSTID of an ABSNMVB, ABSNMVI, ABSNMVR and ABSNMVS)
    Other
};

// Visits the ID fields of a card that point to other cards. Renumbering, validation and reference resolution are
// visitors: each card type lists its references once (CardTable::visitReferences), so that those operations work on
// every card type.
//
// visit() receives the field name, what it points to and the current ID (0 where the field is blank); it returns the
// ID the field should hold afterwards. A visitor that only looks returns `id` unchanged.
class ReferenceVisitor
{
public:
    virtual ~ReferenceVisitor() = default;
    virtual base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) = 0;

    // A reference to an ID that lives in the model of another superelement: the grid points of SECONCT, SEBNDRY and
    // SEEXCLD name those of superelement SEIDA, and the design variables of SEDLINK those of the part they name. The
    // ID is looked up (and, in a renumbering, rewritten) in `Model::superelementModel(superelement)`: the part BEGIN
    // SUPER=superelement when the deck has one, else the residual structure. A visitor that does not follow references
    // across the parts keeps the default, which leaves the ID as it is.
    virtual base::EntityId visitInSuperelement([[maybe_unused]] std::string_view field,
                                               [[maybe_unused]] ReferenceTarget target, base::EntityId id,
                                               [[maybe_unused]] base::EntityId superelement)
    {
        return id;
    }

    // A reference to an ID that lives in a part of the deck named by its BEGIN kind and number: the SET1 and SET3 of
    // an ACPEMCP or ACPMCP1 are in the part BEGIN BULK TRMC=TID, the grid points an
    // MD card places in module n are in BEGIN MODULE=n (or in the primary module that module n copies). The ID is
    // looked up (and, in a renumbering, rewritten) in the models PartScopes::modelsOfPart gives. A visitor that does
    // not follow references across the parts keeps the default, which leaves the ID as it is.
    virtual base::EntityId visitInPart([[maybe_unused]] std::string_view field, [[maybe_unused]] ReferenceTarget target,
                                       base::EntityId id, [[maybe_unused]] std::string_view partKind,
                                       [[maybe_unused]] std::int64_t partId)
    {
        return id;
    }
};

} // namespace oor::model
