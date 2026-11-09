#include "oor/model/references/Renumbering.hpp"

#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/superelements/SuperelementIds.hpp"
#include "oor/model/references/PartScopes.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace oor::model
{

namespace
{

// The ID space a renumbering works on, or the category of the set IDs.
std::optional<base::IdSpace> idSpaceOf(RenumberSpace space) noexcept
{
    switch (space)
    {
    case RenumberSpace::Node:
        return base::IdSpace::Node;
    case RenumberSpace::Element:
        return base::IdSpace::Element;
    case RenumberSpace::SurfaceElement:
        return base::IdSpace::SurfaceElement;
    case RenumberSpace::Property:
        return base::IdSpace::Property;
    case RenumberSpace::Material:
        return base::IdSpace::Material;
    case RenumberSpace::ThermalMaterial:
        return base::IdSpace::ThermalMaterial;
    case RenumberSpace::Coordinate:
        return base::IdSpace::Coordinate;
    case RenumberSpace::Method:
        return base::IdSpace::Method;
    case RenumberSpace::DynamicTable:
        return base::IdSpace::DynamicTable;
    case RenumberSpace::MaterialTable:
        return base::IdSpace::MaterialTable;
    case RenumberSpace::StressTable:
        return base::IdSpace::StressTable;
    case RenumberSpace::HeatTable:
        return base::IdSpace::HeatTable;
    case RenumberSpace::HybridDampingRotorOptionsTable:
        return base::IdSpace::HybridDampingRotorOptionsTable;
    case RenumberSpace::RandomTable:
        return base::IdSpace::RandomTable;
    case RenumberSpace::AeroElement:
        return base::IdSpace::AeroElement;
    case RenumberSpace::AeroProperty:
        return base::IdSpace::AeroProperty;
    case RenumberSpace::LoadCombinationSet:
    case RenumberSpace::ConstraintSet:
    case RenumberSpace::DynamicLoadSet:
    case RenumberSpace::Set:
    case RenumberSpace::FrequencySet:
    case RenumberSpace::ComplexMethod:
    case RenumberSpace::Delay:
    case RenumberSpace::Phase:
    case RenumberSpace::TimeStep:
    case RenumberSpace::Nonlinear:
    case RenumberSpace::Random:
    case RenumberSpace::TransferFunction:
    case RenumberSpace::InitialCondition:
    case RenumberSpace::HeatBoundaryProperty:
    case RenumberSpace::ConvectionProperty:
    case RenumberSpace::ForcedConvectionProperty:
    case RenumberSpace::RadiationMaterial:
    case RenumberSpace::ViewFactorFaceFactor:
    case RenumberSpace::RadiationCavity:
    case RenumberSpace::ContactBody:
    case RenumberSpace::ContactSurface:
    case RenumberSpace::ContactRigid:
    case RenumberSpace::ContactBodyParameters:
    case RenumberSpace::ContactGeometricParameters:
    case RenumberSpace::ContactPhysicalParameters:
    case RenumberSpace::ContactPair:
    case RenumberSpace::ContactTable:
    case RenumberSpace::TrimmingCurve:
    case RenumberSpace::SlidelineSegments:
    case RenumberSpace::SlidelineContact:
    case RenumberSpace::SlidelineFriction:
    case RenumberSpace::NonlinearStep:
    case RenumberSpace::NonlinearHarmonic:
    case RenumberSpace::NonlinearFrequency:
    case RenumberSpace::EquationOfState:
    case RenumberSpace::ShearModel:
    case RenumberSpace::YieldModel:
    case RenumberSpace::FailureModel:
    case RenumberSpace::SpallModel:
    case RenumberSpace::EulerMesh:
    case RenumberSpace::MeshBias:
    case RenumberSpace::EulerBodyForce:
    case RenumberSpace::EulerRegionSet:
    case RenumberSpace::EulerRegionShape:
    case RenumberSpace::Coupling:
    case RenumberSpace::CouplingOptions:
    case RenumberSpace::CohesiveFriction:
    case RenumberSpace::CohesiveFrictionSet:
    case RenumberSpace::CouplingFailure:
    case RenumberSpace::HydrostaticPreset:
    case RenumberSpace::GasBag:
    case RenumberSpace::Leakage:
    case RenumberSpace::Porosity:
    case RenumberSpace::InflatorSet:
    case RenumberSpace::Inflator:
    case RenumberSpace::InflatorGas:
    case RenumberSpace::GasFractions:
    case RenumberSpace::InitialGas:
    case RenumberSpace::DesignVariable:
    case RenumberSpace::DesignEquation:
    case RenumberSpace::DesignResponse:
    case RenumberSpace::DesignConstraintSet:
    case RenumberSpace::PropertyRelation:
    case RenumberSpace::MaterialRelation:
    case RenumberSpace::ConnectivityRelation:
    case RenumberSpace::AeroList:
    case RenumberSpace::AeroFactor:
    case RenumberSpace::AeroFlutterFactor:
    case RenumberSpace::AeroControlSurface:
    case RenumberSpace::AeroTrimSet:
    case RenumberSpace::AeroFlutterSet:
    case RenumberSpace::AeroGustSet:
    case RenumberSpace::DivergenceSet:
    case RenumberSpace::ControlSchedule:
    case RenumberSpace::ControlVector:
    case RenumberSpace::AeroNode:
    case RenumberSpace::AeroScale:
    case RenumberSpace::Superelement:
    case RenumberSpace::MassCombination:
    case RenumberSpace::AeroSpline:
    case RenumberSpace::HybridDampingRotorOptionsSpecification:
    case RenumberSpace::HybridDamping:
    case RenumberSpace::ElementList:
    case RenumberSpace::ModuleMove:
    case RenumberSpace::MultiDimensionalTable:
    case RenumberSpace::FatigueTable:
    case RenumberSpace::ImperfectionCase:
    case RenumberSpace::ImperfectionGeometry:
    case RenumberSpace::GeometricBoundary:
    case RenumberSpace::AdaptivityCriterion:
    case RenumberSpace::AdaptivitySet:
    case RenumberSpace::Rotor:
    case RenumberSpace::DiscreteValueSet:
    case RenumberSpace::UserDefinedName:
    case RenumberSpace::FatigueProperty:
    case RenumberSpace::FatigueLoad:
    case RenumberSpace::FatigueEvent:
    case RenumberSpace::EulerInitialization:
    case RenumberSpace::EulerInitialRegions:
    case RenumberSpace::InitialValueSet:
    case RenumberSpace::HeatLossSet:
    case RenumberSpace::HeatTransferModel:
    case RenumberSpace::CosimulationGroup:
    case RenumberSpace::CosimulationInput:
    case RenumberSpace::CosimulationOutput:
    case RenumberSpace::CosimulationService:
    case RenumberSpace::WettedElement:
    case RenumberSpace::WettedSurface:
    case RenumberSpace::BrakeProperty:
    case RenumberSpace::BrakeMotion:
    case RenumberSpace::BrakeSqueal:
    case RenumberSpace::FrfComponent:
    case RenumberSpace::BushingForceSet:
    case RenumberSpace::LoadStepControl:
    case RenumberSpace::TemperatureGroup:
    case RenumberSpace::MonteCarloSet:
    case RenumberSpace::DeactivatedElementSet:
    case RenumberSpace::TransferPathSet:
    case RenumberSpace::IterativeSolver:
    case RenumberSpace::MetadataGroup:
        break;
    }
    return std::nullopt;
}

std::optional<CardCategory> categoryOf(RenumberSpace space) noexcept
{
    switch (space)
    {
    case RenumberSpace::LoadCombinationSet:
        return CardCategory::Load;
    case RenumberSpace::ConstraintSet:
        return CardCategory::Constraint;
    case RenumberSpace::DynamicLoadSet:
        return CardCategory::DynamicLoad;
    case RenumberSpace::Set:
        return CardCategory::Set;
    case RenumberSpace::FrequencySet:
        return CardCategory::Frequency;
    case RenumberSpace::ComplexMethod:
        return CardCategory::ComplexMethod;
    case RenumberSpace::Delay:
        return CardCategory::Delay;
    case RenumberSpace::Phase:
        return CardCategory::PhaseLead;
    case RenumberSpace::TimeStep:
        return CardCategory::TimeStep;
    case RenumberSpace::Nonlinear:
        return CardCategory::NonlinearStatic;
    case RenumberSpace::Random:
        return CardCategory::Random;
    case RenumberSpace::TransferFunction:
        return CardCategory::TransferFunction;
    case RenumberSpace::InitialCondition:
        return CardCategory::InitialCondition;
    case RenumberSpace::HeatBoundaryProperty:
        return CardCategory::HeatBoundaryProperty;
    case RenumberSpace::ConvectionProperty:
        return CardCategory::ConvectionProperty;
    case RenumberSpace::ForcedConvectionProperty:
        return CardCategory::ForcedConvectionProperty;
    case RenumberSpace::RadiationMaterial:
        return CardCategory::RadiationMaterial;
    case RenumberSpace::ViewFactorFaceFactor:
        return CardCategory::ViewFactorFaceFactor;
    case RenumberSpace::RadiationCavity:
        return CardCategory::RadiationCavity;
    case RenumberSpace::ContactBody:
        return CardCategory::ContactBody;
    case RenumberSpace::ContactSurface:
        return CardCategory::ContactSurface;
    case RenumberSpace::ContactRigid:
        return CardCategory::ContactRigid;
    case RenumberSpace::ContactBodyParameters:
        return CardCategory::ContactBodyParameters;
    case RenumberSpace::ContactGeometricParameters:
        return CardCategory::ContactGeometricParameters;
    case RenumberSpace::ContactPhysicalParameters:
        return CardCategory::ContactPhysicalParameters;
    case RenumberSpace::ContactPair:
        return CardCategory::ContactPair;
    case RenumberSpace::ContactTable:
        return CardCategory::ContactTable;
    case RenumberSpace::TrimmingCurve:
        return CardCategory::TrimmingCurve;
    case RenumberSpace::SlidelineSegments:
        return CardCategory::SlidelineSegments;
    case RenumberSpace::SlidelineContact:
        return CardCategory::SlidelineContact;
    case RenumberSpace::SlidelineFriction:
        return CardCategory::SlidelineFriction;
    case RenumberSpace::NonlinearStep:
        return CardCategory::NonlinearStep;
    case RenumberSpace::NonlinearHarmonic:
        return CardCategory::NonlinearHarmonic;
    case RenumberSpace::NonlinearFrequency:
        return CardCategory::NonlinearFrequency;
    case RenumberSpace::EquationOfState:
        return CardCategory::EquationOfState;
    case RenumberSpace::ShearModel:
        return CardCategory::ShearModel;
    case RenumberSpace::YieldModel:
        return CardCategory::YieldModel;
    case RenumberSpace::FailureModel:
        return CardCategory::FailureModel;
    case RenumberSpace::SpallModel:
        return CardCategory::SpallModel;
    case RenumberSpace::EulerMesh:
        return CardCategory::EulerMesh;
    case RenumberSpace::MeshBias:
        return CardCategory::MeshBias;
    case RenumberSpace::EulerBodyForce:
        return CardCategory::EulerBodyForce;
    case RenumberSpace::EulerRegionSet:
        return CardCategory::EulerRegionSet;
    case RenumberSpace::EulerRegionShape:
        return CardCategory::EulerRegionShape;
    case RenumberSpace::Coupling:
        return CardCategory::Coupling;
    case RenumberSpace::CouplingOptions:
        return CardCategory::CouplingOptions;
    case RenumberSpace::CohesiveFriction:
        return CardCategory::CohesiveFriction;
    case RenumberSpace::CohesiveFrictionSet:
        return CardCategory::CohesiveFrictionSet;
    case RenumberSpace::CouplingFailure:
        return CardCategory::CouplingFailure;
    case RenumberSpace::HydrostaticPreset:
        return CardCategory::HydrostaticPreset;
    case RenumberSpace::GasBag:
        return CardCategory::GasBag;
    case RenumberSpace::Leakage:
        return CardCategory::Leakage;
    case RenumberSpace::Porosity:
        return CardCategory::Porosity;
    case RenumberSpace::InflatorSet:
        return CardCategory::InflatorSet;
    case RenumberSpace::Inflator:
        return CardCategory::Inflator;
    case RenumberSpace::InflatorGas:
        return CardCategory::InflatorGas;
    case RenumberSpace::GasFractions:
        return CardCategory::GasFractions;
    case RenumberSpace::InitialGas:
        return CardCategory::InitialGas;
    case RenumberSpace::DesignVariable:
        return CardCategory::DesignVariable;
    case RenumberSpace::DesignEquation:
        return CardCategory::DesignEquation;
    case RenumberSpace::DesignResponse:
        return CardCategory::DesignResponse;
    case RenumberSpace::DesignConstraintSet:
        return CardCategory::DesignConstraint;
    case RenumberSpace::PropertyRelation:
        return CardCategory::PropertyRelation;
    case RenumberSpace::MaterialRelation:
        return CardCategory::MaterialRelation;
    case RenumberSpace::ConnectivityRelation:
        return CardCategory::ConnectivityRelation;
    case RenumberSpace::AeroList:
        return CardCategory::AeroList;
    case RenumberSpace::AeroFactor:
        return CardCategory::AeroFactor;
    case RenumberSpace::AeroFlutterFactor:
        return CardCategory::AeroFlutterFactor;
    case RenumberSpace::AeroControlSurface:
        return CardCategory::AeroControlSurface;
    case RenumberSpace::AeroTrimSet:
        return CardCategory::AeroTrimSet;
    case RenumberSpace::AeroFlutterSet:
        return CardCategory::AeroFlutterSet;
    case RenumberSpace::AeroGustSet:
        return CardCategory::AeroGustSet;
    case RenumberSpace::DivergenceSet:
        return CardCategory::DivergenceSet;
    case RenumberSpace::ControlSchedule:
        return CardCategory::ControlSchedule;
    case RenumberSpace::ControlVector:
        return CardCategory::ControlVector;
    case RenumberSpace::AeroNode:
        return CardCategory::AeroNode;
    case RenumberSpace::AeroScale:
        return CardCategory::AeroScale;
    case RenumberSpace::Superelement:
        return CardCategory::Superelement;
    case RenumberSpace::MassCombination:
        return CardCategory::MassCombination;
    case RenumberSpace::AeroSpline:
        return CardCategory::AeroSpline;
    case RenumberSpace::HybridDampingRotorOptionsSpecification:
        return CardCategory::HybridDampingRotorOptionsSpecification;
    case RenumberSpace::HybridDamping:
        return CardCategory::HybridDamping;
    case RenumberSpace::ElementList:
        return CardCategory::ElementList;
    case RenumberSpace::ModuleMove:
        return CardCategory::ModuleMove;
    case RenumberSpace::MultiDimensionalTable:
        return CardCategory::MultiDimensionalTable;
    case RenumberSpace::FatigueTable:
        return CardCategory::FatigueTable;
    case RenumberSpace::ImperfectionCase:
        return CardCategory::ImperfectionCase;
    case RenumberSpace::ImperfectionGeometry:
        return CardCategory::ImperfectionGeometry;
    case RenumberSpace::GeometricBoundary:
        return CardCategory::GeometricBoundary;
    case RenumberSpace::AdaptivityCriterion:
        return CardCategory::AdaptivityCriterion;
    case RenumberSpace::AdaptivitySet:
        return CardCategory::AdaptivitySet;
    case RenumberSpace::Rotor:
        return CardCategory::Rotor;
    case RenumberSpace::DiscreteValueSet:
        return CardCategory::DiscreteValueSet;
    case RenumberSpace::UserDefinedName:
        return CardCategory::UserDefinedName;
    case RenumberSpace::FatigueProperty:
        return CardCategory::FatigueProperty;
    case RenumberSpace::FatigueLoad:
        return CardCategory::FatigueLoad;
    case RenumberSpace::FatigueEvent:
        return CardCategory::FatigueEvent;
    case RenumberSpace::EulerInitialization:
        return CardCategory::EulerInitialization;
    case RenumberSpace::EulerInitialRegions:
        return CardCategory::EulerInitialRegions;
    case RenumberSpace::InitialValueSet:
        return CardCategory::InitialValueSet;
    case RenumberSpace::HeatLossSet:
        return CardCategory::HeatLossSet;
    case RenumberSpace::HeatTransferModel:
        return CardCategory::HeatTransferModel;
    case RenumberSpace::CosimulationGroup:
        return CardCategory::CosimulationGroup;
    case RenumberSpace::CosimulationInput:
        return CardCategory::CosimulationInput;
    case RenumberSpace::CosimulationOutput:
        return CardCategory::CosimulationOutput;
    case RenumberSpace::CosimulationService:
        return CardCategory::CosimulationService;
    case RenumberSpace::WettedElement:
        return CardCategory::WettedElement;
    case RenumberSpace::WettedSurface:
        return CardCategory::WettedSurface;
    case RenumberSpace::BrakeProperty:
        return CardCategory::BrakeProperty;
    case RenumberSpace::BrakeMotion:
        return CardCategory::BrakeMotion;
    case RenumberSpace::BrakeSqueal:
        return CardCategory::BrakeSqueal;
    case RenumberSpace::FrfComponent:
        return CardCategory::FrfComponent;
    case RenumberSpace::BushingForceSet:
        return CardCategory::BushingForceSet;
    case RenumberSpace::LoadStepControl:
        return CardCategory::LoadStepControl;
    case RenumberSpace::TemperatureGroup:
        return CardCategory::TemperatureGroup;
    case RenumberSpace::MonteCarloSet:
        return CardCategory::MonteCarloSet;
    case RenumberSpace::DeactivatedElementSet:
        return CardCategory::DeactivatedElementSet;
    case RenumberSpace::TransferPathSet:
        return CardCategory::TransferPathSet;
    case RenumberSpace::IterativeSolver:
        return CardCategory::IterativeSolver;
    case RenumberSpace::MetadataGroup:
        return CardCategory::MetadataGroup;
    default:
        break;
    }
    return std::nullopt;
}

// Are the cards of this type the ones whose IDs a renumbering of `space` changes? A category that is a space is made of
// the cards that number a selection: a set ID shared by several cards (Group), or an ID unique per card type
// (TSTEP 5 and TSTEPNL 5 are the one selection 5, and get the same new ID).
bool tableInSpace(const CardTypeInfo& info, RenumberSpace space) noexcept
{
    if (const auto category = categoryOf(space))
        return (info.idSpace == base::IdSpace::Group || info.idSpace == base::IdSpace::PerCardType) &&
               info.category == *category;
    const auto idSpace = idSpaceOf(space);
    return idSpace && info.idSpace == *idSpace;
}

// Does a reference field with this target point into `space`?
bool referenceInSpace(ReferenceTarget target, RenumberSpace space) noexcept
{
    switch (space)
    {
    case RenumberSpace::Node:
        return target == ReferenceTarget::Node || target == ReferenceTarget::NodeInRange ||
               target == ReferenceTarget::ScalarOrGridPoint;
    case RenumberSpace::Element:
        return target == ReferenceTarget::Element || target == ReferenceTarget::ElementInRange;
    case RenumberSpace::SurfaceElement:
        return target == ReferenceTarget::SurfaceElement || target == ReferenceTarget::SurfaceElementInRange;
    case RenumberSpace::Property:
        return target == ReferenceTarget::Property || target == ReferenceTarget::PropertyInRange;
    case RenumberSpace::Material:
        return target == ReferenceTarget::Material;
    case RenumberSpace::ThermalMaterial:
        return target == ReferenceTarget::ThermalMaterial;
    case RenumberSpace::Coordinate:
        return target == ReferenceTarget::Coordinate;
    case RenumberSpace::Method:
        return target == ReferenceTarget::Method;
    case RenumberSpace::DynamicTable:
        return target == ReferenceTarget::DynamicTable;
    case RenumberSpace::HybridDampingRotorOptionsTable:
        return target == ReferenceTarget::HybridDampingRotorOptionsTable;
    case RenumberSpace::RandomTable:
        return target == ReferenceTarget::RandomTable;
    case RenumberSpace::MaterialTable:
        return target == ReferenceTarget::MaterialTable;
    case RenumberSpace::StressTable:
        return target == ReferenceTarget::StressTable;
    case RenumberSpace::HeatTable:
        return target == ReferenceTarget::HeatTable;
    case RenumberSpace::LoadCombinationSet:
        return target == ReferenceTarget::LoadCombinationSet;
    case RenumberSpace::ConstraintSet:
        return target == ReferenceTarget::ConstraintSet;
    case RenumberSpace::DynamicLoadSet:
        return target == ReferenceTarget::DynamicLoadSet;
    case RenumberSpace::Set:
        return target == ReferenceTarget::Set;
    case RenumberSpace::FrequencySet:
        return target == ReferenceTarget::FrequencySet;
    case RenumberSpace::ComplexMethod:
        return target == ReferenceTarget::ComplexMethod;
    case RenumberSpace::Delay:
        return target == ReferenceTarget::DelaySet;
    case RenumberSpace::Phase:
        return target == ReferenceTarget::PhaseSet;
    case RenumberSpace::TimeStep:
        return target == ReferenceTarget::TimeStepSet;
    case RenumberSpace::Nonlinear:
        return target == ReferenceTarget::NonlinearSet;
    case RenumberSpace::Random:
        return target == ReferenceTarget::RandomSet;
    case RenumberSpace::TransferFunction:
        return target == ReferenceTarget::TransferFunctionSet;
    case RenumberSpace::InitialCondition:
        return target == ReferenceTarget::InitialConditionSet;
    case RenumberSpace::HeatBoundaryProperty:
        return target == ReferenceTarget::HeatBoundaryProperty;
    case RenumberSpace::ConvectionProperty:
        return target == ReferenceTarget::ConvectionProperty;
    case RenumberSpace::ForcedConvectionProperty:
        return target == ReferenceTarget::ForcedConvectionProperty;
    case RenumberSpace::RadiationMaterial:
        return target == ReferenceTarget::RadiationMaterial;
    case RenumberSpace::ViewFactorFaceFactor:
        return target == ReferenceTarget::ViewFactorFaceFactor;
    case RenumberSpace::RadiationCavity:
        return target == ReferenceTarget::RadiationCavity;
    case RenumberSpace::ContactBody:
        return target == ReferenceTarget::ContactBody;
    case RenumberSpace::ContactSurface:
        return target == ReferenceTarget::ContactSurface;
    case RenumberSpace::ContactRigid:
        return target == ReferenceTarget::ContactRigid;
    case RenumberSpace::ContactBodyParameters:
        return target == ReferenceTarget::ContactBodyParameters;
    case RenumberSpace::ContactGeometricParameters:
        return target == ReferenceTarget::ContactGeometricParameters;
    case RenumberSpace::ContactPhysicalParameters:
        return target == ReferenceTarget::ContactPhysicalParameters;
    case RenumberSpace::ContactPair:
        return target == ReferenceTarget::ContactPair;
    case RenumberSpace::ContactTable:
        return target == ReferenceTarget::ContactTable;
    case RenumberSpace::TrimmingCurve:
        return target == ReferenceTarget::TrimmingCurve;
    case RenumberSpace::SlidelineSegments:
        return target == ReferenceTarget::SlidelineSegments;
    case RenumberSpace::SlidelineContact:
        return target == ReferenceTarget::SlidelineContact;
    case RenumberSpace::SlidelineFriction:
        return target == ReferenceTarget::SlidelineFriction;
    case RenumberSpace::NonlinearStep:
        return target == ReferenceTarget::NonlinearStep;
    case RenumberSpace::NonlinearHarmonic:
        return target == ReferenceTarget::NonlinearHarmonic;
    case RenumberSpace::NonlinearFrequency:
        return target == ReferenceTarget::NonlinearFrequency;
    case RenumberSpace::EquationOfState:
        return target == ReferenceTarget::EquationOfState;
    case RenumberSpace::ShearModel:
        return target == ReferenceTarget::ShearModel;
    case RenumberSpace::YieldModel:
        return target == ReferenceTarget::YieldModel;
    case RenumberSpace::FailureModel:
        return target == ReferenceTarget::FailureModel;
    case RenumberSpace::SpallModel:
        return target == ReferenceTarget::SpallModel;
    case RenumberSpace::EulerMesh:
        return target == ReferenceTarget::EulerMesh;
    case RenumberSpace::MeshBias:
        return target == ReferenceTarget::MeshBias;
    case RenumberSpace::EulerBodyForce:
        return target == ReferenceTarget::EulerBodyForce;
    case RenumberSpace::EulerRegionSet:
        return target == ReferenceTarget::EulerRegionSet;
    case RenumberSpace::EulerRegionShape:
        return target == ReferenceTarget::EulerRegionShape;
    case RenumberSpace::Coupling:
        return target == ReferenceTarget::Coupling;
    case RenumberSpace::CouplingOptions:
        return target == ReferenceTarget::CouplingOptions;
    case RenumberSpace::CohesiveFriction:
        return target == ReferenceTarget::CohesiveFriction;
    case RenumberSpace::CohesiveFrictionSet:
        return target == ReferenceTarget::CohesiveFrictionSet;
    case RenumberSpace::CouplingFailure:
        return target == ReferenceTarget::CouplingFailure;
    case RenumberSpace::HydrostaticPreset:
        return target == ReferenceTarget::HydrostaticPreset;
    case RenumberSpace::GasBag:
        return target == ReferenceTarget::GasBag;
    case RenumberSpace::Leakage:
        return target == ReferenceTarget::Leakage;
    case RenumberSpace::Porosity:
        return target == ReferenceTarget::Porosity;
    case RenumberSpace::InflatorSet:
        return target == ReferenceTarget::InflatorSet;
    case RenumberSpace::Inflator:
        return target == ReferenceTarget::Inflator;
    case RenumberSpace::InflatorGas:
        return target == ReferenceTarget::InflatorGas;
    case RenumberSpace::GasFractions:
        return target == ReferenceTarget::GasFractions;
    case RenumberSpace::InitialGas:
        return target == ReferenceTarget::InitialGas;
    case RenumberSpace::DesignVariable:
        return target == ReferenceTarget::DesignVariable;
    case RenumberSpace::DesignEquation:
        return target == ReferenceTarget::DesignEquation;
    case RenumberSpace::DesignResponse:
        return target == ReferenceTarget::DesignResponse;
    case RenumberSpace::DesignConstraintSet:
        return target == ReferenceTarget::DesignConstraintSet;
    case RenumberSpace::PropertyRelation:
        return target == ReferenceTarget::PropertyRelation;
    case RenumberSpace::MaterialRelation:
        return target == ReferenceTarget::MaterialRelation;
    case RenumberSpace::ConnectivityRelation:
        return target == ReferenceTarget::ConnectivityRelation;
    case RenumberSpace::AeroElement:
        return target == ReferenceTarget::AeroElement; // an AeroBox is renumbered through its owner (Rewriter)
    case RenumberSpace::AeroProperty:
        return target == ReferenceTarget::AeroProperty;
    case RenumberSpace::AeroList:
        return target == ReferenceTarget::AeroList;
    case RenumberSpace::AeroFactor:
        return target == ReferenceTarget::AeroFactor;
    case RenumberSpace::AeroFlutterFactor:
        return target == ReferenceTarget::AeroFlutterFactor;
    case RenumberSpace::AeroControlSurface:
        return target == ReferenceTarget::AeroControlSurface;
    case RenumberSpace::AeroTrimSet:
        return target == ReferenceTarget::AeroTrimSet;
    case RenumberSpace::AeroFlutterSet:
        return target == ReferenceTarget::AeroFlutterSet;
    case RenumberSpace::AeroGustSet:
        return target == ReferenceTarget::AeroGustSet;
    case RenumberSpace::DivergenceSet:
        return target == ReferenceTarget::DivergenceSet;
    case RenumberSpace::ControlSchedule:
        return target == ReferenceTarget::ControlSchedule;
    case RenumberSpace::ControlVector:
        return target == ReferenceTarget::ControlVector;
    case RenumberSpace::AeroNode:
        return target == ReferenceTarget::AeroNode;
    case RenumberSpace::AeroScale:
        return target == ReferenceTarget::AeroScale;
    case RenumberSpace::Superelement:
        return target == ReferenceTarget::Superelement;
    case RenumberSpace::MassCombination:
        return target == ReferenceTarget::MassCombination;
    case RenumberSpace::AeroSpline:
        return target == ReferenceTarget::AeroSpline;
    case RenumberSpace::HybridDampingRotorOptionsSpecification:
        return target == ReferenceTarget::HybridDampingRotorOptionsSpecification;
    case RenumberSpace::HybridDamping:
        return target == ReferenceTarget::HybridDamping;
    case RenumberSpace::ElementList:
        return target == ReferenceTarget::ElementList;
    case RenumberSpace::ModuleMove:
        return target == ReferenceTarget::ModuleMove;
    case RenumberSpace::MultiDimensionalTable:
        return target == ReferenceTarget::MultiDimensionalTable;
    case RenumberSpace::FatigueTable:
        return target == ReferenceTarget::FatigueTable;
    case RenumberSpace::ImperfectionCase:
        return target == ReferenceTarget::ImperfectionCase;
    case RenumberSpace::ImperfectionGeometry:
        return target == ReferenceTarget::ImperfectionGeometry;
    case RenumberSpace::GeometricBoundary:
        return target == ReferenceTarget::GeometricBoundary;
    case RenumberSpace::AdaptivityCriterion:
        return target == ReferenceTarget::AdaptivityCriterion;
    case RenumberSpace::AdaptivitySet:
        return target == ReferenceTarget::AdaptivitySet;
    case RenumberSpace::Rotor:
        return target == ReferenceTarget::Rotor;
    case RenumberSpace::DiscreteValueSet:
        return target == ReferenceTarget::DiscreteValueSet;
    case RenumberSpace::UserDefinedName:
        return target == ReferenceTarget::UserDefinedName;
    case RenumberSpace::FatigueProperty:
        return target == ReferenceTarget::FatigueProperty;
    case RenumberSpace::FatigueLoad:
        return target == ReferenceTarget::FatigueLoad;
    case RenumberSpace::FatigueEvent:
        return target == ReferenceTarget::FatigueEvent;
    case RenumberSpace::EulerInitialization:
        return target == ReferenceTarget::EulerInitialization;
    case RenumberSpace::EulerInitialRegions:
        return target == ReferenceTarget::EulerInitialRegions;
    case RenumberSpace::InitialValueSet:
        return target == ReferenceTarget::InitialValueSet;
    case RenumberSpace::HeatLossSet:
        return target == ReferenceTarget::HeatLossSet;
    case RenumberSpace::HeatTransferModel:
        return target == ReferenceTarget::HeatTransferModel;
    case RenumberSpace::CosimulationGroup:
        return target == ReferenceTarget::CosimulationGroup;
    case RenumberSpace::CosimulationInput:
        return target == ReferenceTarget::CosimulationInput;
    case RenumberSpace::CosimulationOutput:
        return target == ReferenceTarget::CosimulationOutput;
    case RenumberSpace::CosimulationService:
        return target == ReferenceTarget::CosimulationService;
    case RenumberSpace::WettedElement:
        return target == ReferenceTarget::WettedElement;
    case RenumberSpace::WettedSurface:
        return target == ReferenceTarget::WettedSurface;
    case RenumberSpace::BrakeProperty:
        return target == ReferenceTarget::BrakeProperty;
    case RenumberSpace::BrakeMotion:
        return target == ReferenceTarget::BrakeMotion;
    case RenumberSpace::BrakeSqueal:
        return target == ReferenceTarget::BrakeSqueal;
    case RenumberSpace::FrfComponent:
        return target == ReferenceTarget::FrfComponent;
    case RenumberSpace::BushingForceSet:
        return target == ReferenceTarget::BushingForceSet;
    case RenumberSpace::LoadStepControl:
        return target == ReferenceTarget::LoadStepControl;
    case RenumberSpace::TemperatureGroup:
        return target == ReferenceTarget::TemperatureGroup;
    case RenumberSpace::MonteCarloSet:
        return target == ReferenceTarget::MonteCarloSet;
    case RenumberSpace::DeactivatedElementSet:
        return target == ReferenceTarget::DeactivatedElementSet;
    case RenumberSpace::TransferPathSet:
        return target == ReferenceTarget::TransferPathSet;
    case RenumberSpace::IterativeSolver:
        return target == ReferenceTarget::IterativeSolver;
    case RenumberSpace::MetadataGroup:
        return target == ReferenceTarget::MetadataGroup;
    }
    return false;
}

// The targets that can name an ID of either of two spaces: IC (a TIC set, or a temperature set of the Load
// category), SDAMPING (a TABDMP1, or a TABLEDi) and BCONTACT (a BCTABL1 or BCTABLE, or a BCONECT). A renumbering of one
// of the two spaces rewrites the reference when the ID belongs to that space and not to the other one (when both have
// it, it cannot be told which was meant).
bool isEitherOfTwoSpaces(ReferenceTarget target, RenumberSpace space) noexcept
{
    if (target == ReferenceTarget::InitialConditionSet)
        return space == RenumberSpace::InitialCondition || space == RenumberSpace::LoadCombinationSet;
    if (target == ReferenceTarget::HybridDampingRotorOptionsTable)
        return space == RenumberSpace::HybridDampingRotorOptionsTable || space == RenumberSpace::DynamicTable;
    if (target == ReferenceTarget::ContactSelection)
        return space == RenumberSpace::ContactTable || space == RenumberSpace::ContactPair;
    return false;
}

// The IDs of the live cards of an ID space.
std::unordered_set<base::EntityId> idsOfSpace(const Model& model, base::IdSpace space)
{
    std::unordered_set<base::EntityId> ids;
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = model.findTable(type);
        if (table == nullptr || registry.entry(type).info.idSpace != space)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                ids.insert(table->idAt(row));
        }
    }
    return ids;
}

// Rewrites the reference fields that point into the renumbered space.
class Rewriter final : public ReferenceVisitor
{
public:
    // `boxes` is the ranges of the aerodynamic boxes as they were before the renumbering of the CAERO cards (null for
    // any other space): an AeroBox reference follows the card that owns the box.
    Rewriter(const Model& model, RenumberSpace space,
             std::function<std::optional<base::EntityId>(base::EntityId)> newIdOf,
             std::unordered_set<base::EntityId> ownSets, std::unordered_set<base::EntityId> otherSets,
             std::unordered_set<base::EntityId> ownEither, std::unordered_set<base::EntityId> otherEither,
             const AeroBoxRanges* boxes)
        : m_model(model),
          m_space(space),
          m_newIdOf(std::move(newIdOf)),
          m_ownSets(std::move(ownSets)),
          m_otherSets(std::move(otherSets)),
          m_ownEither(std::move(ownEither)),
          m_otherEither(std::move(otherEither)),
          m_boxes(boxes)
    {
    }

    base::EntityId visit(std::string_view, ReferenceTarget target, base::EntityId id) override
    {
        if (id == 0)
            return id; // a blank field, the basic coordinate system
        if (target == ReferenceTarget::AeroBox)
            return renumberedBox(id);
        if (id < 0 && m_space != RenumberSpace::MaterialTable)
            return id; // a sentinel; only a material table has a negative ID (the sign is a flag, see signedMapping)
        if (target == ReferenceTarget::Material && m_space == RenumberSpace::ThermalMaterial)
        {
            // The material of a property may be a MAT1 or a MAT4: a renumbering of the thermal materials follows it
            // only where no structural material has that ID (the structural one wins, as when the reference is
            // resolved).
            if (m_ownEither.contains(id) && !m_otherEither.contains(id))
                return m_newIdOf(id).value_or(id);
            return id;
        }
        if (target == ReferenceTarget::StressOrMultiDimensionalTable)
        {
            // The FID of a MATEP names a TABLES1 or TABLEST, or a TABL3Di where no stress table has the ID (the stress
            // table wins, as when the reference is resolved).
            if ((m_space == RenumberSpace::StressTable && m_ownEither.contains(id)) ||
                (m_space == RenumberSpace::MultiDimensionalTable && m_ownEither.contains(id) &&
                 !idsOf(base::IdSpace::StressTable).contains(id)))
                return m_newIdOf(id).value_or(id);
            return id;
        }
        if (target == ReferenceTarget::AeroPanelOrComponent)
        {
            // The CAERO of a SPLINE4 names a CAERO panel, which follows the renumbering of the aerodynamic elements,
            // or the component number of an aerodynamic mesh, which no renumbering changes.
            if (m_space == RenumberSpace::AeroElement && m_ownEither.contains(id))
                return m_newIdOf(id).value_or(id);
            return id;
        }
        if (target == ReferenceTarget::MaterialTable && m_space == RenumberSpace::MultiDimensionalTable)
        {
            // A field that names a TABLEMi or a TABL3Di (the T(RHO) of a MATTUSR): the multi-dimensional tables
            // follow it where no material table has that ID.
            if (m_ownEither.contains(id) && !m_otherEither.contains(id))
                return m_newIdOf(id).value_or(id);
            return id;
        }
        if (target == ReferenceTarget::MaterialTable && m_space == RenumberSpace::MaterialTable &&
            m_otherEither.contains(id) && !m_ownEither.contains(id))
            return id; // the ID of a multi-dimensional table that no material table has
        if (target == ReferenceTarget::FatigueTable && m_space == RenumberSpace::FatigueTable &&
            m_otherEither.contains(id) && !m_ownEither.contains(id))
            return id; // the TID of an FTGLOAD that names a TABLEDi and no TABLFTG
        if (target == ReferenceTarget::FatigueTable && m_space == RenumberSpace::DynamicTable)
        {
            // The TID of an FTGLOAD with no TYPE is a TABLFTG, or a TABLEDi where no TABLFTG has that ID: the dynamic
            // tables follow it there.
            if (m_ownEither.contains(id) && !fatigueTableIds().contains(id))
                return m_newIdOf(id).value_or(id);
            return id;
        }
        if (isEitherOfTwoSpaces(target, m_space))
        {
            if (m_ownEither.contains(id) && !m_otherEither.contains(id))
                return m_newIdOf(id).value_or(id);
            return id;
        }
        if (referenceInSpace(target, m_space) || excitationInSpace(target, id))
            return m_newIdOf(id).value_or(id);
        return id;
    }

    // An ID of another superelement is one of this renumbering only when the superelement lives in this model: the grid
    // points of a SECONCT that name a part are those of the part, which its own renumbering rewrites.
    base::EntityId visitInSuperelement(std::string_view field, ReferenceTarget target, base::EntityId id,
                                       base::EntityId superelement) override
    {
        if (!referenceInSpace(target, m_space) || &m_model.superelementModel(superelement) != &m_model)
            return id;
        return visit(field, target, id);
    }

    // The same for an ID of a part named by its kind and number (the sets of an ACPEMCP in its TRMC part): it is one of
    // this renumbering only when that part is the model renumbered.
    base::EntityId visitInPart(std::string_view field, ReferenceTarget target, base::EntityId id,
                               std::string_view partKind, std::int64_t partId) override
    {
        const std::optional<std::vector<const Model*>> holders = PartScopes::modelsOfPart(m_model, partKind, partId);
        if (!holders || holders->empty() || holders->front() != &m_model)
            return id;
        return visit(field, target, id);
    }

private:
    // The IDs of an ID space (found once per space, when a reference that may name either of two spaces needs them).
    [[nodiscard]] const std::unordered_set<base::EntityId>& idsOf(base::IdSpace space) const
    {
        auto found = m_spaceIds.find(space);
        if (found == m_spaceIds.end())
            found = m_spaceIds.emplace(space, idsOfSpace(m_model, space)).first;
        return found->second;
    }

    // The IDs of the TABLFTG cards (found once, when a renumbering of the dynamic tables meets an FTGLOAD).
    [[nodiscard]] const std::unordered_set<base::EntityId>& fatigueTableIds() const
    {
        if (!m_fatigueTableIds)
            m_fatigueTableIds = setIdsOfCategory(m_model, CardCategory::FatigueTable);
        return *m_fatigueTableIds;
    }

    // A box is numbered from the EID of its CAERO: it keeps its place in the card, so it follows the new EID. A box
    // that no card owns (an AELIST of grid points, a box of a card that is not there) is left as it is.
    [[nodiscard]] base::EntityId renumberedBox(base::EntityId box) const
    {
        if (m_boxes == nullptr)
            return box;
        const AeroBoxRange* owner = m_boxes->owner(box);
        if (owner == nullptr)
            return box;
        const base::EntityId newEid = m_newIdOf(owner->panelId).value_or(owner->panelId);
        const std::int64_t moved =
            static_cast<std::int64_t>(newEid) + (static_cast<std::int64_t>(box) - owner->panelId);
        return base::toEntityId(moved).value_or(box);
    }

    // An EXCITEID names a set of dynamic loads (DAREA) or of static loads: it follows the renumbering of the category
    // that has the set, and is left alone when both have one (it cannot be told which was meant).
    [[nodiscard]] bool excitationInSpace(ReferenceTarget target, base::EntityId id) const
    {
        if (target != ReferenceTarget::ExcitationSet && target != ReferenceTarget::ExcitationDefined)
            return false;
        if (m_space != RenumberSpace::LoadCombinationSet && m_space != RenumberSpace::DynamicLoadSet)
            return false;
        return m_ownSets.contains(id) && !m_otherSets.contains(id);
    }

    const Model& m_model;
    RenumberSpace m_space;
    std::function<std::optional<base::EntityId>(base::EntityId)> m_newIdOf;
    std::unordered_set<base::EntityId> m_ownSets;
    std::unordered_set<base::EntityId> m_otherSets;
    std::unordered_set<base::EntityId> m_ownEither; // IDs of the space, for a reference that may be to either space
    std::unordered_set<base::EntityId> m_otherEither;
    mutable std::optional<std::unordered_set<base::EntityId>> m_fatigueTableIds;
    mutable std::unordered_map<base::IdSpace, std::unordered_set<base::EntityId>> m_spaceIds;
    const AeroBoxRanges* m_boxes;
};

// The mapping applied to an ID of the space. A material table may have a negative ID (TABLEM1 -5: the sign makes the
// table a thermal strain table, and the T(A) field of a MATT1, MATT2 or MATT8 names it with the sign): the table number
// is the absolute value, which the mapping renumbers, and the sign stays.
std::optional<base::EntityId> signedMapping(RenumberSpace space, const IdMapping& mapping, base::EntityId id)
{
    if (space == RenumberSpace::MaterialTable && id < 0)
    {
        const std::optional<base::EntityId> updated = mapping(-id);
        return updated ? std::optional<base::EntityId>(-*updated) : std::nullopt;
    }
    return mapping(id);
}

// The IDs in use in the space and the ID each gets. Throws when a new ID is not valid or two IDs would become the same.
std::unordered_map<base::EntityId, base::EntityId> planNewIds(const Model& model, RenumberSpace space,
                                                              const IdMapping& mapping)
{
    const CardRegistry& registry = CardRegistry::instance();
    std::unordered_map<base::EntityId, base::EntityId> newIds;
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = model.findTable(type);
        if (table == nullptr || !tableInSpace(registry.entry(type).info, space))
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const base::EntityId old = table->idAt(row);
            if (space == RenumberSpace::Superelement && old == 0)
                continue; // the residual structure keeps its number
            if (newIds.contains(old))
                continue;
            const base::EntityId updated = signedMapping(space, mapping, old).value_or(old);
            if (updated == 0 || (updated < 0) != (old < 0))
                throw base::UsageError("renumbering ID " + std::to_string(old) + " gives " + std::to_string(updated) +
                                       ": IDs are positive");
            newIds.emplace(old, updated);
        }
    }
    return newIds;
}

// No two IDs may become the same, whether both change or one stays.
void requireDistinct(const std::unordered_map<base::EntityId, base::EntityId>& newIds)
{
    std::unordered_map<base::EntityId, base::EntityId> origin;
    for (const auto& [old, updated] : newIds)
    {
        const auto [slot, inserted] = origin.emplace(updated, old);
        if (!inserted)
            throw base::DuplicateIdError("renumbering gives ID " + std::to_string(updated) + " to both " +
                                         std::to_string(slot->second) + " and " + std::to_string(old));
    }
}

// Rewrites, in the residual structure that holds a part, the references that name an ID of that part: the grid points
// of a SECONCT, the design variables of an SEDLINK. They are looked up in the part (visitInSuperelement), so the
// renumbering of the part's own IDs has to reach them there, and no other reference of the residual structure changes.
class PartReferenceRewriter final : public ReferenceVisitor
{
public:
    PartReferenceRewriter(const Model& owner, const Model& part, RenumberSpace space,
                          std::function<std::optional<base::EntityId>(base::EntityId)> newIdOf)
        : m_owner(owner), m_part(part), m_space(space), m_newIdOf(std::move(newIdOf))
    {
    }

    base::EntityId visit(std::string_view, ReferenceTarget, base::EntityId id) override { return id; }

    base::EntityId visitInSuperelement(std::string_view, ReferenceTarget target, base::EntityId id,
                                       base::EntityId superelement) override
    {
        if (id <= 0 || !referenceInSpace(target, m_space) || &m_owner.superelementModel(superelement) != &m_part)
            return id;
        return m_newIdOf(id).value_or(id);
    }

    base::EntityId visitInPart(std::string_view, ReferenceTarget target, base::EntityId id, std::string_view partKind,
                               std::int64_t partId) override
    {
        if (id <= 0 || !referenceInSpace(target, m_space))
            return id;
        const std::optional<std::vector<const Model*>> holders = PartScopes::modelsOfPart(m_owner, partKind, partId);
        if (!holders || holders->empty() || holders->front() != &m_part)
            return id;
        return m_newIdOf(id).value_or(id);
    }

private:
    const Model& m_owner;
    const Model& m_part;
    RenumberSpace m_space;
    std::function<std::optional<base::EntityId>(base::EntityId)> m_newIdOf;
};

// Gives the cards of the space their new IDs in `model` and rewrites the references of its cards and of its Case
// Control to them. Returns the number of cards whose ID changed.
std::size_t rewriteModel(Model& model, RenumberSpace space, const IdMapping& mapping,
                         const std::unordered_map<base::EntityId, base::EntityId>& newIds)
{
    const CardRegistry& registry = CardRegistry::instance();

    // The boxes of the CAERO cards are numbered from their EIDs: the ranges are taken while the EIDs are still the old
    // ones.
    std::optional<AeroBoxRanges> boxes;
    if (space == RenumberSpace::AeroElement)
        boxes.emplace(model);

    std::unordered_set<base::EntityId> ownSets;
    std::unordered_set<base::EntityId> otherSets;
    if (space == RenumberSpace::LoadCombinationSet || space == RenumberSpace::DynamicLoadSet)
    {
        const bool isLoad = space == RenumberSpace::LoadCombinationSet;
        ownSets = setIdsOfCategory(model, isLoad ? CardCategory::Load : CardCategory::DynamicLoad);
        otherSets = setIdsOfCategory(model, isLoad ? CardCategory::DynamicLoad : CardCategory::Load);
    }

    // The two spaces an IC or an SDAMPING may name (see isEitherOfTwoSpaces): the IDs of this one and of the other one,
    // taken before the cards get their new IDs.
    std::unordered_set<base::EntityId> ownEither;
    std::unordered_set<base::EntityId> otherEither;
    switch (space)
    {
    case RenumberSpace::InitialCondition:
        ownEither = setIdsOfCategory(model, CardCategory::InitialCondition);
        otherEither = setIdsOfCategory(model, CardCategory::Load);
        break;
    case RenumberSpace::LoadCombinationSet:
        ownEither = setIdsOfCategory(model, CardCategory::Load);
        otherEither = setIdsOfCategory(model, CardCategory::InitialCondition);
        break;
    case RenumberSpace::HybridDampingRotorOptionsTable:
        ownEither = idsOfSpace(model, base::IdSpace::HybridDampingRotorOptionsTable);
        otherEither = idsOfSpace(model, base::IdSpace::DynamicTable);
        break;
    case RenumberSpace::DynamicTable:
        ownEither = idsOfSpace(model, base::IdSpace::DynamicTable);
        otherEither = idsOfSpace(model, base::IdSpace::HybridDampingRotorOptionsTable);
        break;
    case RenumberSpace::ThermalMaterial:
        ownEither = idsOfSpace(model, base::IdSpace::ThermalMaterial);
        otherEither = idsOfSpace(model, base::IdSpace::Material);
        break;
    case RenumberSpace::ContactTable:
        ownEither = setIdsOfCategory(model, CardCategory::ContactTable);
        otherEither = setIdsOfCategory(model, CardCategory::ContactPair);
        break;
    case RenumberSpace::ContactPair:
        ownEither = setIdsOfCategory(model, CardCategory::ContactPair);
        otherEither = setIdsOfCategory(model, CardCategory::ContactTable);
        break;
    case RenumberSpace::MaterialTable:
        ownEither = idsOfSpace(model, base::IdSpace::MaterialTable);
        otherEither = setIdsOfCategory(model, CardCategory::MultiDimensionalTable);
        break;
    case RenumberSpace::MultiDimensionalTable:
        ownEither = setIdsOfCategory(model, CardCategory::MultiDimensionalTable);
        otherEither = idsOfSpace(model, base::IdSpace::MaterialTable);
        break;
    case RenumberSpace::AeroElement: // the CAERO of a SPLINE4 may also name an aerodynamic mesh component
        ownEither = idsOfSpace(model, base::IdSpace::AeroElement);
        break;
    case RenumberSpace::StressTable: // the FID of a MATEP may also name a TABL3Di
        ownEither = idsOfSpace(model, base::IdSpace::StressTable);
        break;
    case RenumberSpace::FatigueTable:
        ownEither = setIdsOfCategory(model, CardCategory::FatigueTable);
        otherEither = idsOfSpace(model, base::IdSpace::DynamicTable);
        break;
    default:
        break;
    }

    std::size_t changed = 0;
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        CardTable* table = model.findTable(type);
        if (table == nullptr || !tableInSpace(registry.entry(type).info, space))
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const base::EntityId old = table->idAt(row);
            const auto found = newIds.find(old);
            if (found == newIds.end() || found->second == old)
                continue;
            table->setIdAt(row, found->second);
            ++changed;
        }
    }

    // The references: to an ID of a card that exists, its new ID; to another ID (a dangling reference), whatever the
    // mapping makes of it.
    const auto newIdOf = [&](base::EntityId id) -> std::optional<base::EntityId>
    {
        if (const auto found = newIds.find(id); found != newIds.end())
            return found->second;
        return signedMapping(space, mapping, id);
    };
    Rewriter rewriter(model, space, newIdOf, std::move(ownSets), std::move(otherSets), std::move(ownEither),
                      std::move(otherEither), boxes ? &*boxes : nullptr);
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                table->visitReferences(row, rewriter);
        }
    }
    // The Case Control commands that hold a set ID (SPC, LOAD, METHOD, ...) follow the cards.
    model.control().caseControl.visitReferences(rewriter);
    model.reindex();
    model.noteContentChange();
    // Node and element IDs are what the connectivity tables of the analysis layer are built from.
    if (space == RenumberSpace::Node || space == RenumberSpace::Element)
        model.noteStructureChange();
    return changed;
}

std::size_t renumberOne(Model& model, RenumberSpace space, const IdMapping& mapping)
{
    const std::unordered_map<base::EntityId, base::EntityId> newIds = planNewIds(model, space, mapping);
    requireDistinct(newIds);
    const std::size_t changed = rewriteModel(model, space, mapping, newIds);

    // What the residual structure names in this part follows (the residual structure itself has already been
    // rewritten when it is the model renumbered).
    if (Model* owner = model.owner())
    {
        const auto newIdOf = [&](base::EntityId id) -> std::optional<base::EntityId>
        {
            if (const auto found = newIds.find(id); found != newIds.end())
                return found->second;
            return signedMapping(space, mapping, id);
        };
        PartReferenceRewriter rewriter(*owner, model, space, newIdOf);
        const CardRegistry& registry = CardRegistry::instance();
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            CardTable* table = owner->findTable(type);
            if (table == nullptr)
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (table->rows().isAlive(row))
                    table->visitReferences(row, rewriter);
            }
        }
        owner->noteContentChange();
    }
    return changed;
}

// The superelement numbers belong to the whole deck: one list of the numbers in use (the parts, the cards of the
// family, the SEID of the grid points), one new number for each, and every model of the deck (the residual structure
// and its parts) takes it, with the BEGIN lines of the parts.
std::size_t renumberSuperelements(Model& root, const IdMapping& mapping)
{
    std::set<base::EntityId> inUse;
    const SuperelementIds defined(root);
    inUse.insert(defined.sorted().begin(), defined.sorted().end());
    const CardRegistry& registry = CardRegistry::instance();
    for (const std::unique_ptr<Model>& part : root.parts())
    {
        // A card of the family inside a part (the solver ignores it) has a number the rest of the deck may not have.
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const CardTable* table = part->findTable(type);
            if (table == nullptr || !tableInSpace(registry.entry(type).info, RenumberSpace::Superelement))
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (table->rows().isAlive(row) && table->idAt(row) > 0)
                    inUse.insert(table->idAt(row));
            }
        }
    }
    std::unordered_map<base::EntityId, base::EntityId> newIds;
    for (const base::EntityId old : inUse)
    {
        const base::EntityId updated = mapping(old).value_or(old);
        if (updated <= 0)
            throw base::UsageError("renumbering superelement " + std::to_string(old) + " gives " +
                                   std::to_string(updated) + ": superelement numbers are positive");
        newIds.emplace(old, updated);
    }
    requireDistinct(newIds);

    std::size_t changed = rewriteModel(root, RenumberSpace::Superelement, mapping, newIds);
    for (const std::unique_ptr<Model>& part : root.parts())
    {
        changed += rewriteModel(*part, RenumberSpace::Superelement, mapping, newIds);
        const PartInfo& info = *part->partInfo();
        if (info.kind == "SuperelementPart")
        {
            if (const auto found = newIds.find(static_cast<base::EntityId>(info.id)); found != newIds.end())
            {
                if (found->second != info.id)
                {
                    part->setPartId(found->second);
                    ++changed;
                }
            }
        }
    }
    return changed;
}

} // namespace

std::size_t Renumbering::apply(Model& model, RenumberSpace space, const MappingFactory& factory, bool includeParts)
{
    if (space == RenumberSpace::Superelement && model.owner() != nullptr)
        throw base::UsageError(
            "the superelement numbers belong to the whole deck: renumber them on the model that holds the parts");
    // A part is renumbered together with what the residual structure names in it, so the whole deck is put back on a
    // failure.
    Model& root = model.root();
    Model backup(root);
    try
    {
        if (space == RenumberSpace::Superelement)
            return renumberSuperelements(model, factory(model));
        std::size_t changed = renumberOne(model, space, factory(model));
        if (includeParts)
        {
            for (const std::unique_ptr<Model>& part : model.parts())
                changed += renumberOne(*part, space, factory(*part));
        }
        return changed;
    }
    catch (...)
    {
        root.replaceContents(std::move(backup));
        throw;
    }
}

MappingFactory Renumbering::offset(base::EntityId offset)
{
    return [offset](const Model&) -> IdMapping
    {
        return [offset](base::EntityId id) -> std::optional<base::EntityId>
        {
            const std::int64_t shifted = static_cast<std::int64_t>(id) + offset;
            return base::toEntityId(shifted);
        };
    };
}

MappingFactory Renumbering::consecutive(RenumberSpace space, base::EntityId start, base::EntityId step)
{
    return [space, start, step](const Model& model) -> IdMapping
    {
        if (space == RenumberSpace::Superelement)
        {
            // The numbers belong to no card in particular (a part, the SEID of a grid point): ascending order.
            auto numbered = std::make_shared<std::unordered_map<base::EntityId, base::EntityId>>();
            std::int64_t following = start;
            const SuperelementIds defined(model);
            for (const base::EntityId id : defined.sorted())
            {
                numbered->emplace(id,
                                  static_cast<base::EntityId>(std::min<std::int64_t>(following, base::MAX_ENTITY_ID)));
                following += step;
            }
            return [numbered](base::EntityId id) -> std::optional<base::EntityId>
            {
                const auto found = numbered->find(id);
                if (found == numbered->end())
                    return std::nullopt;
                return found->second;
            };
        }
        // The IDs in file order (first appearance of each).
        struct Entry
        {
            std::uint32_t key;
            base::EntityId id;
        };
        std::vector<Entry> entries;
        const CardRegistry& registry = CardRegistry::instance();
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const CardTable* table = model.findTable(type);
            if (table == nullptr || !tableInSpace(registry.entry(type).info, space))
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (table->rows().isAlive(row))
                {
                    // A material table with a negative ID has the number of its absolute value (see signedMapping).
                    const base::EntityId id = table->idAt(row);
                    entries.push_back(Entry{table->rows().keyAt(row).value,
                                            space == RenumberSpace::MaterialTable && id < 0 ? -id : id});
                }
            }
        }
        std::sort(entries.begin(), entries.end(),
                  [](const Entry& left, const Entry& right) { return left.key < right.key; });
        auto assigned = std::make_shared<std::unordered_map<base::EntityId, base::EntityId>>();
        std::int64_t next = start;
        for (const Entry& entry : entries)
        {
            if (assigned
                    ->emplace(entry.id, static_cast<base::EntityId>(std::min<std::int64_t>(next, base::MAX_ENTITY_ID)))
                    .second)
                next += step;
        }
        return [assigned](base::EntityId id) -> std::optional<base::EntityId>
        {
            const auto found = assigned->find(id);
            if (found == assigned->end())
                return std::nullopt;
            return found->second;
        };
    };
}

MappingFactory Renumbering::fromFunction(IdMapping mapping)
{
    return [mapping = std::move(mapping)](const Model&) { return mapping; };
}

} // namespace oor::model
