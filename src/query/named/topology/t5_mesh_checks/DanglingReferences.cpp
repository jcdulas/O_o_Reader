#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"

namespace oor::query
{

namespace
{

// The kind of card a reference field names, in one word.
const char* categoryOf(model::ReferenceTarget target)
{
    using model::ReferenceTarget;
    switch (target)
    {
    case ReferenceTarget::Node:
    case ReferenceTarget::NodeInRange:
    case ReferenceTarget::ScalarOrGridPoint:
        return "node";
    case ReferenceTarget::Element:
    case ReferenceTarget::ElementInRange:
    case ReferenceTarget::SurfaceElement:
    case ReferenceTarget::SurfaceElementInRange:
        return "element";
    case ReferenceTarget::Property:
    case ReferenceTarget::PropertyInRange:
        return "property";
    case ReferenceTarget::Material:
    case ReferenceTarget::ThermalMaterial:
        return "material";
    case ReferenceTarget::Coordinate:
        return "coord";
    case ReferenceTarget::DynamicTable:
    case ReferenceTarget::MaterialTable:
    case ReferenceTarget::StressTable:
    case ReferenceTarget::StressOrMultiDimensionalTable:
    case ReferenceTarget::HeatTable:
    case ReferenceTarget::HybridDampingRotorOptionsTable:
    case ReferenceTarget::RandomTable:
        return "table";
    case ReferenceTarget::Method:
    case ReferenceTarget::ComplexMethod:
        return "method";
    case ReferenceTarget::LoadCombinationSet:
    case ReferenceTarget::DynamicLoadSet:
    case ReferenceTarget::ExcitationSet:
    case ReferenceTarget::ExcitationDefined:
        return "loadSet";
    case ReferenceTarget::ConstraintSet:
        return "constraintSet";
    case ReferenceTarget::Set:
        return "set";
    case ReferenceTarget::HeatBoundaryProperty:
    case ReferenceTarget::ConvectionProperty:
    case ReferenceTarget::ForcedConvectionProperty:
    case ReferenceTarget::RadiationMaterial:
    case ReferenceTarget::ViewFactorFaceFactor:
    case ReferenceTarget::RadiationCavity:
    case ReferenceTarget::TemperatureGroup:
        return "thermalSet";
    case ReferenceTarget::ContactBody:
    case ReferenceTarget::ContactSurface:
    case ReferenceTarget::ContactRigid:
    case ReferenceTarget::ContactBodyParameters:
    case ReferenceTarget::ContactGeometricParameters:
    case ReferenceTarget::ContactPhysicalParameters:
    case ReferenceTarget::ContactPair:
    case ReferenceTarget::ContactTable:
    case ReferenceTarget::ContactSelection:
    case ReferenceTarget::TrimmingCurve:
    case ReferenceTarget::SlidelineSegments:
    case ReferenceTarget::SlidelineContact:
    case ReferenceTarget::SlidelineFriction:
    case ReferenceTarget::BrakeProperty:
    case ReferenceTarget::BrakeMotion:
    case ReferenceTarget::BrakeSqueal:
        return "contactSet";
    case ReferenceTarget::FrequencySet:
    case ReferenceTarget::DelaySet:
    case ReferenceTarget::PhaseSet:
    case ReferenceTarget::TimeStepSet:
    case ReferenceTarget::NonlinearSet:
    case ReferenceTarget::RandomSet:
    case ReferenceTarget::TransferFunctionSet:
    case ReferenceTarget::InitialConditionSet:
    case ReferenceTarget::NonlinearStep:
    case ReferenceTarget::NonlinearHarmonic:
    case ReferenceTarget::NonlinearFrequency:
        return "analysisSet";
    case ReferenceTarget::EquationOfState:
    case ReferenceTarget::ShearModel:
    case ReferenceTarget::YieldModel:
    case ReferenceTarget::FailureModel:
    case ReferenceTarget::SpallModel:
    case ReferenceTarget::EulerMesh:
    case ReferenceTarget::MeshBias:
    case ReferenceTarget::EulerBodyForce:
    case ReferenceTarget::EulerRegionSet:
    case ReferenceTarget::EulerRegionShape:
    case ReferenceTarget::Coupling:
    case ReferenceTarget::CouplingOptions:
    case ReferenceTarget::CohesiveFriction:
    case ReferenceTarget::CohesiveFrictionSet:
    case ReferenceTarget::CouplingFailure:
    case ReferenceTarget::HydrostaticPreset:
    case ReferenceTarget::GasBag:
    case ReferenceTarget::Leakage:
    case ReferenceTarget::Porosity:
    case ReferenceTarget::InflatorSet:
    case ReferenceTarget::Inflator:
    case ReferenceTarget::InflatorGas:
    case ReferenceTarget::GasFractions:
    case ReferenceTarget::InitialGas:
    case ReferenceTarget::EulerInitialization:
    case ReferenceTarget::EulerInitialRegions:
    case ReferenceTarget::InitialValueSet:
    case ReferenceTarget::HeatLossSet:
    case ReferenceTarget::HeatTransferModel:
        return "explicitSet";
    case ReferenceTarget::DesignVariable:
    case ReferenceTarget::DesignEquation:
    case ReferenceTarget::DesignResponse:
    case ReferenceTarget::DesignConstraintSet:
    case ReferenceTarget::PropertyRelation:
    case ReferenceTarget::MaterialRelation:
    case ReferenceTarget::ConnectivityRelation:
    case ReferenceTarget::DiscreteValueSet:
        return "designSet";
    case ReferenceTarget::AeroElement:
    case ReferenceTarget::AeroPanelOrComponent:
    case ReferenceTarget::AeroBox:
    case ReferenceTarget::AeroProperty:
    case ReferenceTarget::AeroList:
    case ReferenceTarget::AeroFactor:
    case ReferenceTarget::AeroFlutterFactor:
    case ReferenceTarget::AeroControlSurface:
    case ReferenceTarget::AeroTrimSet:
    case ReferenceTarget::AeroFlutterSet:
    case ReferenceTarget::AeroGustSet:
    case ReferenceTarget::DivergenceSet:
    case ReferenceTarget::ControlSchedule:
    case ReferenceTarget::ControlVector:
    case ReferenceTarget::AeroNode:
    case ReferenceTarget::AeroScale:
    case ReferenceTarget::AeroSpline:
        return "aeroSet";
    case ReferenceTarget::HybridDampingRotorOptionsSpecification:
    case ReferenceTarget::HybridDamping:
        return "analysisSet";
    case ReferenceTarget::ElementList:
        return "set";
    case ReferenceTarget::MultiDimensionalTable:
    case ReferenceTarget::FatigueTable:
        return "table";
    case ReferenceTarget::ModuleMove:
    case ReferenceTarget::ImperfectionCase:
    case ReferenceTarget::ImperfectionGeometry:
    case ReferenceTarget::GeometricBoundary:
    case ReferenceTarget::AdaptivityCriterion:
    case ReferenceTarget::AdaptivitySet:
    case ReferenceTarget::Rotor:
    case ReferenceTarget::UserDefinedName:
    case ReferenceTarget::FatigueProperty:
    case ReferenceTarget::FatigueLoad:
    case ReferenceTarget::FatigueEvent:
    case ReferenceTarget::CosimulationGroup:
    case ReferenceTarget::CosimulationInput:
    case ReferenceTarget::CosimulationOutput:
    case ReferenceTarget::CosimulationService:
    case ReferenceTarget::WettedElement:
    case ReferenceTarget::WettedSurface:
    case ReferenceTarget::FrfComponent:
    case ReferenceTarget::BushingForceSet:
    case ReferenceTarget::LoadStepControl:
    case ReferenceTarget::MonteCarloSet:
    case ReferenceTarget::DeactivatedElementSet:
    case ReferenceTarget::TransferPathSet:
    case ReferenceTarget::IterativeSolver:
    case ReferenceTarget::MetadataGroup:
        return "analysisSet";
    case ReferenceTarget::Superelement:
    case ReferenceTarget::MassIncrement:
    case ReferenceTarget::MassCombination:
        return "superelementSet";
    case ReferenceTarget::Other:
        break;
    }
    return "other";
}

// danglingReferences: the fields that name a card that does not exist.
class DanglingReferencesQuery final : public QueryBase
{
public:
    DanglingReferencesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto references = analysis::ComponentCache::of(model).references(model);
        std::vector<std::string> card;
        std::vector<std::int32_t> id;
        std::vector<std::string> field;
        std::vector<std::string> category;
        std::vector<std::int32_t> targetId;
        for (const analysis::DanglingReference& entry : references->dangling())
        {
            if (entry.inCaseControl)
            {
                card.push_back(entry.scope);
                id.push_back(static_cast<std::int32_t>(entry.scopeNumber));
            }
            else
            {
                const model::CardTable* table = model.findTable(entry.from.table);
                card.emplace_back(typeName(entry.from.table));
                id.push_back(table->idAt(entry.from.row));
            }
            field.push_back(entry.field);
            category.emplace_back(categoryOf(entry.target));
            targetId.push_back(entry.id);
        }
        TableBuilder builder;
        builder.dictionary("card", card);
        builder.int32("id", std::move(id));
        builder.dictionary("field", field);
        builder.dictionary("targetCategory", category);
        builder.int32("targetId", std::move(targetId));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "danglingReferences";
        info.family = "T5";
        info.description =
            "The reference fields that name a card that does not exist: the card (its type and ID), the field, the "
            "kind of "
            "card it names (node, element, property, material, coord, table, method, loadSet, constraintSet, set, "
            "analysisSet, contactSet, explicitSet) and the ID it holds. A command of the Case Control is listed "
            "with the section it is in "
            "(card "
            "SUBCASE or GLOBAL, id the subcase number). The fields that may name nothing (the end of a THRU range, a "
            "scalar "
            "point no SPOINT defines) are not reported. In the order of the cards, then the Case Control.";
        info.defaultSort = "(the order of the cards)";
        ColumnListBuilder columns;
        columns.dictionary("card").int32("id").dictionary("field").dictionary("targetCategory").int32("targetId");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDanglingReferencesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DanglingReferencesQuery>());
}

} // namespace oor::query
