#include "oor/model/references/ReferenceChecker.hpp"

#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/aero/aeroQuad4/AeroQuad4Table.hpp"
#include "oor/model/cards/aero/aeroTria3/AeroTria3Table.hpp"
#include "oor/model/cards/superelements/SuperelementIds.hpp"
#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryTable.hpp"
#include "oor/model/references/PartScopes.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>

namespace oor::model
{

namespace
{

// The ID space of a reference target, when it is one.
std::optional<base::IdSpace> spaceOf(ReferenceTarget target) noexcept
{
    switch (target)
    {
    case ReferenceTarget::Node:
    case ReferenceTarget::NodeInRange:
    case ReferenceTarget::ScalarOrGridPoint:
        return base::IdSpace::Node;
    case ReferenceTarget::Element:
    case ReferenceTarget::ElementInRange:
        return base::IdSpace::Element;
    case ReferenceTarget::SurfaceElement:
    case ReferenceTarget::SurfaceElementInRange:
        return base::IdSpace::SurfaceElement;
    case ReferenceTarget::Property:
    case ReferenceTarget::PropertyInRange:
        return base::IdSpace::Property;
    case ReferenceTarget::Material:
        return base::IdSpace::Material;
    case ReferenceTarget::ThermalMaterial:
        return base::IdSpace::ThermalMaterial;
    case ReferenceTarget::Coordinate:
        return base::IdSpace::Coordinate;
    case ReferenceTarget::Method:
        return base::IdSpace::Method;
    case ReferenceTarget::HybridDampingRotorOptionsTable:
        return base::IdSpace::HybridDampingRotorOptionsTable;
    case ReferenceTarget::RandomTable:
        return base::IdSpace::RandomTable;
    case ReferenceTarget::AeroElement:
    case ReferenceTarget::AeroPanelOrComponent:
        return base::IdSpace::AeroElement;
    case ReferenceTarget::AeroProperty:
        return base::IdSpace::AeroProperty;
    case ReferenceTarget::DynamicTable:
        return base::IdSpace::DynamicTable;
    case ReferenceTarget::MaterialTable:
        return base::IdSpace::MaterialTable;
    case ReferenceTarget::StressTable:
    case ReferenceTarget::StressOrMultiDimensionalTable:
        return base::IdSpace::StressTable;
    case ReferenceTarget::HeatTable:
        return base::IdSpace::HeatTable;
    case ReferenceTarget::LoadCombinationSet:
    case ReferenceTarget::ConstraintSet:
    case ReferenceTarget::DynamicLoadSet:
    case ReferenceTarget::ExcitationSet:
    case ReferenceTarget::ExcitationDefined:
    case ReferenceTarget::ComplexMethod:
    case ReferenceTarget::FrequencySet:
    case ReferenceTarget::DelaySet:
    case ReferenceTarget::PhaseSet:
    case ReferenceTarget::TimeStepSet:
    case ReferenceTarget::NonlinearSet:
    case ReferenceTarget::RandomSet:
    case ReferenceTarget::TransferFunctionSet:
    case ReferenceTarget::InitialConditionSet:
    case ReferenceTarget::Set: // a SET1 or SET3 ID: found with the set cards (categoriesOfSet)
    case ReferenceTarget::HeatBoundaryProperty:
    case ReferenceTarget::ConvectionProperty:
    case ReferenceTarget::ForcedConvectionProperty:
    case ReferenceTarget::RadiationMaterial:
    case ReferenceTarget::ViewFactorFaceFactor:
    case ReferenceTarget::RadiationCavity: // the ID of the cards of a thermal category (categoriesOfSet)
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
    case ReferenceTarget::NonlinearStep:
    case ReferenceTarget::NonlinearHarmonic:
    case ReferenceTarget::NonlinearFrequency:
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
    case ReferenceTarget::DesignVariable:
    case ReferenceTarget::DesignEquation:
    case ReferenceTarget::DesignResponse:
    case ReferenceTarget::DesignConstraintSet:
    case ReferenceTarget::PropertyRelation:
    case ReferenceTarget::MaterialRelation:
    case ReferenceTarget::ConnectivityRelation:
    case ReferenceTarget::AeroBox: // found through the ranges of the CAERO cards (resolveReference)
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
    case ReferenceTarget::Superelement:    // a part, a card of the family or the SEID of a grid (SuperelementIds)
    case ReferenceTarget::MassIncrement:   // a part that carries a MASSID (massIncrementsOf)
    case ReferenceTarget::MassCombination: // the ID of the cards of its category (categoriesOfSet)
    case ReferenceTarget::AeroSpline:
    case ReferenceTarget::HybridDampingRotorOptionsSpecification:
    case ReferenceTarget::HybridDamping:
    case ReferenceTarget::ElementList:
    case ReferenceTarget::ModuleMove:
    case ReferenceTarget::MultiDimensionalTable:
    case ReferenceTarget::FatigueTable:
    case ReferenceTarget::ImperfectionCase:
    case ReferenceTarget::ImperfectionGeometry:
    case ReferenceTarget::GeometricBoundary:
    case ReferenceTarget::AdaptivityCriterion:
    case ReferenceTarget::AdaptivitySet:
    case ReferenceTarget::Rotor:
    case ReferenceTarget::DiscreteValueSet:
    case ReferenceTarget::UserDefinedName:
    case ReferenceTarget::FatigueProperty:
    case ReferenceTarget::FatigueLoad:
    case ReferenceTarget::FatigueEvent:
    case ReferenceTarget::EulerInitialization:
    case ReferenceTarget::EulerInitialRegions:
    case ReferenceTarget::InitialValueSet:
    case ReferenceTarget::HeatLossSet:
    case ReferenceTarget::HeatTransferModel:
    case ReferenceTarget::CosimulationGroup:
    case ReferenceTarget::CosimulationInput:
    case ReferenceTarget::CosimulationOutput:
    case ReferenceTarget::CosimulationService:
    case ReferenceTarget::WettedElement:
    case ReferenceTarget::WettedSurface:
    case ReferenceTarget::BrakeProperty:
    case ReferenceTarget::BrakeMotion:
    case ReferenceTarget::BrakeSqueal:
    case ReferenceTarget::FrfComponent:
    case ReferenceTarget::BushingForceSet:
    case ReferenceTarget::LoadStepControl:
    case ReferenceTarget::TemperatureGroup:
    case ReferenceTarget::MonteCarloSet:
    case ReferenceTarget::DeactivatedElementSet:
    case ReferenceTarget::TransferPathSet:
    case ReferenceTarget::IterativeSolver:
    case ReferenceTarget::MetadataGroup:
    case ReferenceTarget::Other:
        break;
    }
    return std::nullopt;
}

// The excitation set IDs that live cards define (the EXCITEID of each LSEQ), with the first card that defines each.
std::unordered_map<base::EntityId, CardLocation> definedExcitations(const Model& model)
{
    std::unordered_map<base::EntityId, CardLocation> defined;
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = model.findTable(type);
        if (table == nullptr || !table->definesExcitationIds())
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            if (const base::EntityId id = table->definedExcitationId(row); id != 0)
                defined.emplace(id, CardLocation{type, row});
        }
    }
    return defined;
}

// The first AEQUAD4 or AETRIA3 whose component (CMPID) is `component`.
std::optional<CardLocation> aeroMeshComponent(const Model& model, base::EntityId component)
{
    if (const AeroQuad4Table* quads = model.findTable<AeroQuad4Table>())
    {
        for (std::uint32_t row = 0; row < quads->rows().rowCount(); ++row)
        {
            if (quads->rows().isAlive(row) && quads->card(row).componentId == component)
                return CardLocation{typeIndexOf<AeroQuad4Table>(), row};
        }
    }
    if (const AeroTria3Table* triangles = model.findTable<AeroTria3Table>())
    {
        for (std::uint32_t row = 0; row < triangles->rows().rowCount(); ++row)
        {
            if (triangles->rows().isAlive(row) && triangles->card(row).componentId == component)
                return CardLocation{typeIndexOf<AeroTria3Table>(), row};
        }
    }
    return std::nullopt;
}

// The targets that name a grid or scalar point: the ones an EXTRN of a part defines.
bool isPointTarget(ReferenceTarget target) noexcept
{
    return target == ReferenceTarget::Node || target == ReferenceTarget::NodeInRange ||
           target == ReferenceTarget::ScalarOrGridPoint;
}

// Tells whether the target of a reference exists. A set ID is looked up in the set of the IDs of its category, built
// once per index; the other targets in the ID index of their space.
//
// Two things belong to the superelements. A superelement number exists when the deck defines it (SuperelementIds). The
// exterior points an EXTRN lists exist in its part, which has no GRID cards for them: the card says they are there.
// And a part may name what other models of the deck have (PartScopes::sharedModels: the image of a mirrored
// superelement, the base model of a mass increment, the primary of a copied module, the other sub-models of a TPA).
class ExistenceIndex
{
public:
    // `modelSetIds`: the set IDs from Model::setIdsOf (kept by the model while cards are added one by one) rather than
    // a pass over the cards of the category made for this index.
    explicit ExistenceIndex(const Model& model, bool followSharedModels = true, bool modelSetIds = false)
        : m_model(model), m_followSharedModels(followSharedModels), m_modelSetIds(modelSetIds)
    {
    }

    // False only for a reference that can be resolved and is not.
    bool exists(ReferenceTarget target, base::EntityId id)
    {
        if (existsHere(target, id))
            return true;
        if (isPointTarget(target) && isExternalPoint(id))
            return true;
        for (const std::unique_ptr<ExistenceIndex>& shared : sharedIndexes())
        {
            if (shared->existsHere(target, id))
                return true;
        }
        return false;
    }

private:
    bool existsHere(ReferenceTarget target, base::EntityId id)
    {
        if (target == ReferenceTarget::Superelement)
        {
            if (!m_superelements)
                m_superelements.emplace(m_model);
            return m_superelements->contains(id);
        }
        if (target == ReferenceTarget::MassIncrement)
        {
            if (!m_increments)
                m_increments = massIncrementsOf(m_model);
            return m_increments->contains(id);
        }
        if (const std::span<const CardCategory> categories = categoriesOfSet(target); !categories.empty())
        {
            for (const CardCategory category : categories)
            {
                if (m_modelSetIds)
                {
                    if (m_model.setIdsOf(category).contains(id))
                        return true;
                    continue;
                }
                auto found = m_setIds.find(category);
                if (found == m_setIds.end())
                    found = m_setIds.emplace(category, setIdsOfCategory(m_model, category)).first;
                if (found->second.contains(id))
                    return true;
            }
            // An EXCITEID may also be the one an LSEQ defines.
            if (target == ReferenceTarget::ExcitationSet)
            {
                if (!m_excitations)
                    m_excitations = definedExcitations(m_model);
                return m_excitations->contains(id);
            }
            return false;
        }
        return !spaceOf(target) || resolveReference(m_model, target, id).has_value();
    }

    // The exterior points an EXTRN of this model lists.
    bool isExternalPoint(base::EntityId id)
    {
        if (!m_externalPoints)
        {
            const SuperelementExternalBoundaryTable* extrn = m_model.findTable<SuperelementExternalBoundaryTable>();
            m_externalPoints = extrn != nullptr ? extrn->connectionPoints() : std::vector<IdRun>{};
        }
        const std::vector<IdRun>& runs = *m_externalPoints;
        const auto after = std::upper_bound(runs.begin(), runs.end(), id,
                                            [](base::EntityId value, const IdRun& run) { return value < run.first; });
        return after != runs.begin() && std::prev(after)->last >= id;
    }

    const std::vector<std::unique_ptr<ExistenceIndex>>& sharedIndexes()
    {
        if (m_followSharedModels && !m_sharedLooked)
        {
            m_sharedLooked = true;
            for (const Model* shared : PartScopes::sharedModels(m_model))
                m_shared.push_back(std::make_unique<ExistenceIndex>(*shared, false));
        }
        return m_shared;
    }

    const Model& m_model;
    bool m_followSharedModels;
    bool m_modelSetIds;
    bool m_sharedLooked = false;
    std::vector<std::unique_ptr<ExistenceIndex>> m_shared;
    std::unordered_map<CardCategory, std::unordered_set<base::EntityId>> m_setIds;
    std::optional<std::unordered_map<base::EntityId, CardLocation>> m_excitations;
    std::optional<SuperelementIds> m_superelements;
    std::optional<std::unordered_set<base::EntityId>> m_increments;
    std::optional<std::vector<IdRun>> m_externalPoints;
};

// The indexes of the models a card's references go to: its own, and the part or residual structure of a superelement.
class IndexOfModels
{
public:
    ExistenceIndex& of(const Model& model)
    {
        auto found = m_indexes.find(&model);
        if (found == m_indexes.end())
            found = m_indexes.emplace(&model, std::make_unique<ExistenceIndex>(model)).first;
        return *found->second;
    }

private:
    std::unordered_map<const Model*, std::unique_ptr<ExistenceIndex>> m_indexes;
};

// "in superelement 3" for a reference to an ID of another superelement, nothing for the others.
std::string superelementNote(bool scoped, base::EntityId superelement)
{
    if (!scoped)
        return {};
    return superelement == 0 ? " in the residual structure" : " in superelement " + std::to_string(superelement);
}

// " in TRMC 3" for a reference to an ID of a part named by its kind and number.
std::string partNote(std::string_view partKind, std::int64_t partId)
{
    return " in " + std::string(partKind) + " " + std::to_string(partId);
}

// Whether an ID that a card places in the part `partKind`=`partId` exists there (or in the models that part shares its
// IDs with): nothing when the deck cannot tell (a module that is not in the deck).
std::optional<bool> existsInPart(const Model& model, IndexOfModels& indexes, ReferenceTarget target, base::EntityId id,
                                 std::string_view partKind, std::int64_t partId)
{
    const std::optional<std::vector<const Model*>> holders = PartScopes::modelsOfPart(model, partKind, partId);
    if (!holders)
        return std::nullopt;
    for (const Model* holder : *holders)
    {
        if (indexes.of(*holder).exists(target, id))
            return true;
    }
    return false;
}

// Reports each unresolved reference of the card it is told about; leaves every ID as it is.
class Checker final : public ReferenceVisitor
{
public:
    // `severity` and `code` are those of the diagnostics: an error for a card, a warning for a Case Control command (it
    // may point at a card of a type the library does not read yet).
    // `modelSetIds`: the own model's set IDs from Model::setIdsOf (the check of a card being added: unresolvedAt).
    Checker(const Model& model, base::DiagnosticSink& sink, base::Severity severity = base::Severity::Error,
            std::string code = "model.unresolved-reference", bool modelSetIds = false)
        : m_model(model), m_own(model, true, modelSetIds), m_sink(sink), m_severity(severity), m_code(std::move(code))
    {
    }

    // The card the next references are of: its type (a neutral name) and its ID. A diagnostic names the card and the
    // field by the keywords of the dictionary of the model.
    void setCard(std::string_view type, base::EntityId id)
    {
        m_cardType = std::string(type);
        m_name = std::string(type);
        m_where = m_name + " " + std::to_string(id);
    }
    // Where the next references are, when it is not a card: "SUBCASE 3".
    void setScope(SubcaseKind kind, std::int64_t number)
    {
        m_cardType.clear();
        m_name = subcaseLabel(kind);
        m_where = number == 0 ? "the " + m_name + " section" : m_name + " " + std::to_string(number);
    }
    [[nodiscard]] std::size_t unresolved() const noexcept { return m_unresolved; }
    // The fields of the references reported, by their names in the code, in the order of the diagnostics.
    [[nodiscard]] const std::vector<std::string>& reportedFields() const noexcept { return m_reportedFields; }

    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        check(m_own, field, target, id, false, 0);
        return id;
    }

    base::EntityId visitInSuperelement(std::string_view field, ReferenceTarget target, base::EntityId id,
                                       base::EntityId superelement) override
    {
        const Model& holder = m_model.superelementModel(superelement);
        check(&holder == &m_model ? m_own : m_others.of(holder), field, target, id, true, superelement);
        return id;
    }

    base::EntityId visitInPart(std::string_view field, ReferenceTarget target, base::EntityId id,
                               std::string_view partKind, std::int64_t partId) override
    {
        if (id <= 0 || isExemptReference(target, id) || mayPointAtNothing(target))
            return id;
        if (existsInPart(m_model, m_others, target, id, partKind, partId).value_or(true))
            return id;
        const std::string label = labelOf(field);
        ++m_unresolved;
        m_reportedFields.emplace_back(field);
        m_sink.report(base::Diagnostic{m_severity, m_code,
                                       "field " + label + " of " + m_where + " refers to " + std::to_string(id) +
                                           partNote(partKind, partId) + ", which does not exist",
                                       base::SourceLocation{"", 0, m_name, label}});
        return id;
    }

private:
    void check(ExistenceIndex& index, std::string_view field, ReferenceTarget target, base::EntityId id, bool scoped,
               base::EntityId superelement)
    {
        if (isExemptReference(target, id) || mayPointAtNothing(target) || index.exists(target, id))
            return;
        const std::string label = labelOf(field);
        ++m_unresolved;
        m_reportedFields.emplace_back(field);
        m_sink.report(base::Diagnostic{m_severity, m_code,
                                       "field " + label + " of " + m_where + " refers to " + std::to_string(id) +
                                           superelementNote(scoped, superelement) + ", which does not exist",
                                       base::SourceLocation{"", 0, m_name, label}});
    }

    // The name of a field in a diagnostic: the keyword of a field of a card, or of a Case Control command.
    [[nodiscard]] std::string labelOf(std::string_view field) const
    {
        if (m_cardType.empty())
            return std::string(field);
        return std::string(field);
    }

    const Model& m_model;
    ExistenceIndex m_own;
    IndexOfModels m_others;
    base::DiagnosticSink& m_sink;
    base::Severity m_severity;
    std::string m_code;
    std::string m_cardType; // empty for a scope of the Case Control
    std::string m_name;
    std::string m_where;
    std::size_t m_unresolved = 0;
    std::vector<std::string> m_reportedFields;
};

// Hands every non-blank reference field to a callback, with whether it resolves.
class Scanner final : public ReferenceVisitor
{
public:
    Scanner(const Model& model, const std::function<void(const ReferenceUse&)>& callback)
        : m_model(model), m_callback(callback)
    {
    }

    void setCard(std::uint16_t type, std::uint32_t row)
    {
        m_use = ReferenceUse{};
        m_use.location = CardLocation{type, row};
    }
    void setScope(SubcaseKind kind, std::int64_t number)
    {
        m_use = ReferenceUse{};
        m_use.scope = subcaseKindName(kind);
        m_use.scopeNumber = number;
        m_use.inCaseControl = true;
    }

    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        report(m_own, field, target, id, false, 0);
        return id;
    }

    base::EntityId visitInSuperelement(std::string_view field, ReferenceTarget target, base::EntityId id,
                                       base::EntityId superelement) override
    {
        const Model& holder = m_model.superelementModel(superelement);
        report(&holder == &m_model ? m_own : m_others.of(holder), field, target, id, true, superelement);
        return id;
    }

    base::EntityId visitInPart(std::string_view field, ReferenceTarget target, base::EntityId id,
                               std::string_view partKind, std::int64_t partId) override
    {
        if (id == 0)
            return id;
        m_use.field = field;
        m_use.target = target;
        m_use.id = id;
        m_use.scoped = true;
        m_use.superelement = 0;
        m_use.resolved = isExemptReference(target, id) || mayPointAtNothing(target) ||
                         existsInPart(m_model, m_others, target, id, partKind, partId).value_or(true);
        m_callback(m_use);
        return id;
    }

private:
    void report(ExistenceIndex& index, std::string_view field, ReferenceTarget target, base::EntityId id, bool scoped,
                base::EntityId superelement)
    {
        if (id == 0)
            return;
        m_use.field = field;
        m_use.target = target;
        m_use.id = id;
        m_use.scoped = scoped;
        m_use.superelement = superelement;
        m_use.resolved = isExemptReference(target, id) || mayPointAtNothing(target) || index.exists(target, id);
        m_callback(m_use);
    }

    const Model& m_model;
    ExistenceIndex m_own{m_model};
    IndexOfModels m_others;
    const std::function<void(const ReferenceUse&)>& m_callback;
    ReferenceUse m_use;
};

} // namespace

std::span<const CardCategory> categoriesOfSet(ReferenceTarget target) noexcept
{
    static constexpr CardCategory LOAD[] = {CardCategory::Load};
    static constexpr CardCategory CONSTRAINT[] = {CardCategory::Constraint};
    static constexpr CardCategory DYNAMIC_LOAD[] = {CardCategory::DynamicLoad};
    static constexpr CardCategory EXCITATION[] = {CardCategory::DynamicLoad, CardCategory::Load};
    static constexpr CardCategory BULK_SET[] = {CardCategory::Set};
    static constexpr CardCategory FREQUENCY[] = {CardCategory::Frequency};
    static constexpr CardCategory COMPLEX_METHOD[] = {CardCategory::ComplexMethod};
    static constexpr CardCategory DELAY[] = {CardCategory::Delay};
    static constexpr CardCategory PHASE[] = {CardCategory::PhaseLead};
    static constexpr CardCategory TIME_STEP[] = {CardCategory::TimeStep};
    static constexpr CardCategory NONLINEAR[] = {CardCategory::NonlinearStatic};
    static constexpr CardCategory RANDOM[] = {CardCategory::Random};
    static constexpr CardCategory TRANSFER_FUNCTION[] = {CardCategory::TransferFunction};
    // IC selects TIC cards, or (in heat transfer) the temperature sets TEMP and TEMPD, which are of the Load category.
    static constexpr CardCategory INITIAL_CONDITION[] = {CardCategory::InitialCondition, CardCategory::Load};
    static constexpr CardCategory HEAT_BOUNDARY_PROPERTY[] = {CardCategory::HeatBoundaryProperty};
    static constexpr CardCategory CONVECTION_PROPERTY[] = {CardCategory::ConvectionProperty};
    static constexpr CardCategory FORCED_CONVECTION_PROPERTY[] = {CardCategory::ForcedConvectionProperty};
    static constexpr CardCategory RADIATION_MATERIAL[] = {CardCategory::RadiationMaterial};
    static constexpr CardCategory VIEW_FACTOR[] = {CardCategory::ViewFactorFaceFactor};
    static constexpr CardCategory RADIATION_CAVITY[] = {CardCategory::RadiationCavity};
    static constexpr CardCategory CONTACT_BODY[] = {CardCategory::ContactBody};
    // A BSID names the surface of a body; the SOL 700 form of BCGRID (a cloud of grid points) is one too, and it is a
    // card of the body category (its other form is a body of its own).
    static constexpr CardCategory CONTACT_SURFACE[] = {CardCategory::ContactSurface, CardCategory::ContactBody};
    static constexpr CardCategory CONTACT_RIGID[] = {CardCategory::ContactRigid};
    static constexpr CardCategory CONTACT_BODY_PARAMETERS[] = {CardCategory::ContactBodyParameters};
    static constexpr CardCategory CONTACT_GEOMETRIC_PARAMETERS[] = {CardCategory::ContactGeometricParameters};
    static constexpr CardCategory CONTACT_PHYSICAL_PARAMETERS[] = {CardCategory::ContactPhysicalParameters};
    static constexpr CardCategory CONTACT_PAIR[] = {CardCategory::ContactPair};
    static constexpr CardCategory CONTACT_TABLE[] = {CardCategory::ContactTable};
    static constexpr CardCategory CONTACT_SELECTION[] = {CardCategory::ContactTable, CardCategory::ContactPair};
    static constexpr CardCategory TRIMMING_CURVE[] = {CardCategory::TrimmingCurve};
    static constexpr CardCategory SLIDELINE_SEGMENTS[] = {CardCategory::SlidelineSegments};
    static constexpr CardCategory SLIDELINE_CONTACT[] = {CardCategory::SlidelineContact};
    static constexpr CardCategory SLIDELINE_FRICTION[] = {CardCategory::SlidelineFriction};
    static constexpr CardCategory NONLINEAR_STEP[] = {CardCategory::NonlinearStep};
    static constexpr CardCategory NONLINEAR_HARMONIC[] = {CardCategory::NonlinearHarmonic};
    static constexpr CardCategory NONLINEAR_FREQUENCY[] = {CardCategory::NonlinearFrequency};
    static constexpr CardCategory EQUATION_OF_STATE[] = {CardCategory::EquationOfState};
    static constexpr CardCategory SHEAR_MODEL[] = {CardCategory::ShearModel};
    static constexpr CardCategory YIELD_MODEL[] = {CardCategory::YieldModel};
    static constexpr CardCategory FAILURE_MODEL[] = {CardCategory::FailureModel};
    static constexpr CardCategory SPALL_MODEL[] = {CardCategory::SpallModel};
    static constexpr CardCategory EULER_MESH[] = {CardCategory::EulerMesh};
    static constexpr CardCategory MESH_BIAS[] = {CardCategory::MeshBias};
    static constexpr CardCategory EULER_BODY_FORCE[] = {CardCategory::EulerBodyForce};
    static constexpr CardCategory EULER_REGION_SET[] = {CardCategory::EulerRegionSet};
    static constexpr CardCategory EULER_REGION_SHAPE[] = {CardCategory::EulerRegionShape};
    static constexpr CardCategory COUPLING[] = {CardCategory::Coupling};
    static constexpr CardCategory COUPLING_OPTIONS[] = {CardCategory::CouplingOptions};
    static constexpr CardCategory COHESIVE_FRICTION[] = {CardCategory::CohesiveFriction};
    static constexpr CardCategory COHESIVE_FRICTION_SET[] = {CardCategory::CohesiveFrictionSet};
    static constexpr CardCategory COUPLING_FAILURE[] = {CardCategory::CouplingFailure};
    static constexpr CardCategory HYDROSTATIC_PRESET[] = {CardCategory::HydrostaticPreset};
    static constexpr CardCategory GAS_BAG[] = {CardCategory::GasBag};
    static constexpr CardCategory LEAKAGE[] = {CardCategory::Leakage};
    static constexpr CardCategory POROSITY[] = {CardCategory::Porosity};
    static constexpr CardCategory INFLATOR_SET[] = {CardCategory::InflatorSet};
    static constexpr CardCategory INFLATOR[] = {CardCategory::Inflator};
    static constexpr CardCategory INFLATOR_GAS[] = {CardCategory::InflatorGas};
    static constexpr CardCategory GAS_FRACTIONS[] = {CardCategory::GasFractions};
    static constexpr CardCategory INITIAL_GAS[] = {CardCategory::InitialGas};
    static constexpr CardCategory DESIGN_VARIABLE[] = {CardCategory::DesignVariable};
    static constexpr CardCategory DESIGN_EQUATION[] = {CardCategory::DesignEquation};
    static constexpr CardCategory DESIGN_RESPONSE[] = {CardCategory::DesignResponse};
    static constexpr CardCategory DESIGN_CONSTRAINT_SET[] = {CardCategory::DesignConstraint};
    static constexpr CardCategory PROPERTY_RELATION[] = {CardCategory::PropertyRelation};
    static constexpr CardCategory MATERIAL_RELATION[] = {CardCategory::MaterialRelation};
    static constexpr CardCategory CONNECTIVITY_RELATION[] = {CardCategory::ConnectivityRelation};
    static constexpr CardCategory AERO_LIST[] = {CardCategory::AeroList};
    static constexpr CardCategory AERO_FACTOR[] = {CardCategory::AeroFactor};
    static constexpr CardCategory FLUTTER_FACTOR[] = {CardCategory::AeroFlutterFactor};
    static constexpr CardCategory AERO_CONTROL_SURFACE[] = {CardCategory::AeroControlSurface};
    static constexpr CardCategory TRIM_SET[] = {CardCategory::AeroTrimSet};
    static constexpr CardCategory FLUTTER_SET[] = {CardCategory::AeroFlutterSet};
    static constexpr CardCategory GUST_SET[] = {CardCategory::AeroGustSet};
    static constexpr CardCategory DIVERGENCE_SET[] = {CardCategory::DivergenceSet};
    static constexpr CardCategory CONTROL_SCHEDULE[] = {CardCategory::ControlSchedule};
    static constexpr CardCategory CONTROL_VECTOR[] = {CardCategory::ControlVector};
    static constexpr CardCategory AERO_NODE[] = {CardCategory::AeroNode};
    static constexpr CardCategory AERO_SCALE[] = {CardCategory::AeroScale};
    static constexpr CardCategory SUPERELEMENT[] = {CardCategory::Superelement};
    static constexpr CardCategory MASS_COMBINATION[] = {CardCategory::MassCombination};
    static constexpr CardCategory AERO_SPLINE[] = {CardCategory::AeroSpline};
    static constexpr CardCategory DAMPING_SPECIFICATION[] = {CardCategory::HybridDampingRotorOptionsSpecification};
    static constexpr CardCategory HYBRID_DAMPING[] = {CardCategory::HybridDamping};
    static constexpr CardCategory ELEMENT_LIST[] = {CardCategory::ElementList};
    static constexpr CardCategory MODULE_MOVE[] = {CardCategory::ModuleMove};
    static constexpr CardCategory MULTI_DIMENSIONAL_TABLE[] = {CardCategory::MultiDimensionalTable};
    // The TID of an FTGLOAD with no TYPE is a TABLFTG, or a TABLEDi: the dynamic
    // tables are in the category Table.
    static constexpr CardCategory FATIGUE_TABLE[] = {CardCategory::FatigueTable, CardCategory::Table};
    static constexpr CardCategory IMPERFECTION_CASE[] = {CardCategory::ImperfectionCase};
    static constexpr CardCategory IMPERFECTION_GEOMETRY[] = {CardCategory::ImperfectionGeometry};
    static constexpr CardCategory GEOMETRIC_BOUNDARY[] = {CardCategory::GeometricBoundary};
    static constexpr CardCategory ADAPTIVITY_CRITERION[] = {CardCategory::AdaptivityCriterion};
    static constexpr CardCategory ADAPTIVITY_SET[] = {CardCategory::AdaptivitySet};
    static constexpr CardCategory ROTOR[] = {CardCategory::Rotor};
    static constexpr CardCategory DISCRETE_VALUE_SET[] = {CardCategory::DiscreteValueSet};
    static constexpr CardCategory USER_DEFINED_NAME[] = {CardCategory::UserDefinedName};
    static constexpr CardCategory FATIGUE_PROPERTY[] = {CardCategory::FatigueProperty};
    static constexpr CardCategory FATIGUE_LOAD[] = {CardCategory::FatigueLoad};
    static constexpr CardCategory FATIGUE_EVENT[] = {CardCategory::FatigueEvent};
    static constexpr CardCategory EULER_INITIALIZATION[] = {CardCategory::EulerInitialization};
    static constexpr CardCategory EULER_INITIAL_REGIONS[] = {CardCategory::EulerInitialRegions};
    static constexpr CardCategory INITIAL_VALUE_SET[] = {CardCategory::InitialValueSet};
    static constexpr CardCategory HEAT_LOSS_SET[] = {CardCategory::HeatLossSet};
    static constexpr CardCategory HEAT_TRANSFER_MODEL[] = {CardCategory::HeatTransferModel};
    static constexpr CardCategory COSIMULATION_GROUP[] = {CardCategory::CosimulationGroup};
    static constexpr CardCategory COSIMULATION_INPUT[] = {CardCategory::CosimulationInput};
    static constexpr CardCategory COSIMULATION_OUTPUT[] = {CardCategory::CosimulationOutput};
    static constexpr CardCategory COSIMULATION_SERVICE[] = {CardCategory::CosimulationService};
    static constexpr CardCategory WETTED_ELEMENT[] = {CardCategory::WettedElement};
    static constexpr CardCategory WETTED_SURFACE[] = {CardCategory::WettedSurface};
    static constexpr CardCategory BRAKE_PROPERTY[] = {CardCategory::BrakeProperty};
    static constexpr CardCategory BRAKE_MOTION[] = {CardCategory::BrakeMotion};
    static constexpr CardCategory BRAKE_SQUEAL[] = {CardCategory::BrakeSqueal};
    static constexpr CardCategory FRF_COMPONENT[] = {CardCategory::FrfComponent};
    static constexpr CardCategory BUSHING_FORCE_SET[] = {CardCategory::BushingForceSet};
    static constexpr CardCategory LOAD_STEP_CONTROL[] = {CardCategory::LoadStepControl};
    static constexpr CardCategory TEMPERATURE_GROUP[] = {CardCategory::TemperatureGroup};
    static constexpr CardCategory MONTE_CARLO_SET[] = {CardCategory::MonteCarloSet};
    static constexpr CardCategory DEACTIVATED_ELEMENT_SET[] = {CardCategory::DeactivatedElementSet};
    static constexpr CardCategory TRANSFER_PATH_SET[] = {CardCategory::TransferPathSet};
    static constexpr CardCategory ITERATIVE_SOLVER[] = {CardCategory::IterativeSolver};
    static constexpr CardCategory METADATA_GROUP[] = {CardCategory::MetadataGroup};
    switch (target)
    {
    case ReferenceTarget::Set:
        return BULK_SET;
    case ReferenceTarget::LoadCombinationSet:
        return LOAD;
    case ReferenceTarget::ConstraintSet:
        return CONSTRAINT;
    case ReferenceTarget::DynamicLoadSet:
        return DYNAMIC_LOAD;
    case ReferenceTarget::ExcitationSet:
        return EXCITATION;
    case ReferenceTarget::FrequencySet:
        return FREQUENCY;
    case ReferenceTarget::ComplexMethod:
        return COMPLEX_METHOD;
    case ReferenceTarget::DelaySet:
        return DELAY;
    case ReferenceTarget::PhaseSet:
        return PHASE;
    case ReferenceTarget::TimeStepSet:
        return TIME_STEP;
    case ReferenceTarget::NonlinearSet:
        return NONLINEAR;
    case ReferenceTarget::RandomSet:
        return RANDOM;
    case ReferenceTarget::TransferFunctionSet:
        return TRANSFER_FUNCTION;
    case ReferenceTarget::InitialConditionSet:
        return INITIAL_CONDITION;
    case ReferenceTarget::HeatBoundaryProperty:
        return HEAT_BOUNDARY_PROPERTY;
    case ReferenceTarget::ConvectionProperty:
        return CONVECTION_PROPERTY;
    case ReferenceTarget::ForcedConvectionProperty:
        return FORCED_CONVECTION_PROPERTY;
    case ReferenceTarget::RadiationMaterial:
        return RADIATION_MATERIAL;
    case ReferenceTarget::ViewFactorFaceFactor:
        return VIEW_FACTOR;
    case ReferenceTarget::RadiationCavity:
        return RADIATION_CAVITY;
    case ReferenceTarget::ContactBody:
        return CONTACT_BODY;
    case ReferenceTarget::ContactSurface:
        return CONTACT_SURFACE;
    case ReferenceTarget::ContactRigid:
        return CONTACT_RIGID;
    case ReferenceTarget::ContactBodyParameters:
        return CONTACT_BODY_PARAMETERS;
    case ReferenceTarget::ContactGeometricParameters:
        return CONTACT_GEOMETRIC_PARAMETERS;
    case ReferenceTarget::ContactPhysicalParameters:
        return CONTACT_PHYSICAL_PARAMETERS;
    case ReferenceTarget::ContactPair:
        return CONTACT_PAIR;
    case ReferenceTarget::ContactTable:
        return CONTACT_TABLE;
    case ReferenceTarget::ContactSelection:
        return CONTACT_SELECTION;
    case ReferenceTarget::TrimmingCurve:
        return TRIMMING_CURVE;
    case ReferenceTarget::SlidelineSegments:
        return SLIDELINE_SEGMENTS;
    case ReferenceTarget::SlidelineContact:
        return SLIDELINE_CONTACT;
    case ReferenceTarget::SlidelineFriction:
        return SLIDELINE_FRICTION;
    case ReferenceTarget::NonlinearStep:
        return NONLINEAR_STEP;
    case ReferenceTarget::NonlinearHarmonic:
        return NONLINEAR_HARMONIC;
    case ReferenceTarget::NonlinearFrequency:
        return NONLINEAR_FREQUENCY;
    case ReferenceTarget::EquationOfState:
        return EQUATION_OF_STATE;
    case ReferenceTarget::ShearModel:
        return SHEAR_MODEL;
    case ReferenceTarget::YieldModel:
        return YIELD_MODEL;
    case ReferenceTarget::FailureModel:
        return FAILURE_MODEL;
    case ReferenceTarget::SpallModel:
        return SPALL_MODEL;
    case ReferenceTarget::EulerMesh:
        return EULER_MESH;
    case ReferenceTarget::MeshBias:
        return MESH_BIAS;
    case ReferenceTarget::EulerBodyForce:
        return EULER_BODY_FORCE;
    case ReferenceTarget::EulerRegionSet:
        return EULER_REGION_SET;
    case ReferenceTarget::EulerRegionShape:
        return EULER_REGION_SHAPE;
    case ReferenceTarget::Coupling:
        return COUPLING;
    case ReferenceTarget::CouplingOptions:
        return COUPLING_OPTIONS;
    case ReferenceTarget::CohesiveFriction:
        return COHESIVE_FRICTION;
    case ReferenceTarget::CohesiveFrictionSet:
        return COHESIVE_FRICTION_SET;
    case ReferenceTarget::CouplingFailure:
        return COUPLING_FAILURE;
    case ReferenceTarget::HydrostaticPreset:
        return HYDROSTATIC_PRESET;
    case ReferenceTarget::GasBag:
        return GAS_BAG;
    case ReferenceTarget::Leakage:
        return LEAKAGE;
    case ReferenceTarget::Porosity:
        return POROSITY;
    case ReferenceTarget::InflatorSet:
        return INFLATOR_SET;
    case ReferenceTarget::Inflator:
        return INFLATOR;
    case ReferenceTarget::InflatorGas:
        return INFLATOR_GAS;
    case ReferenceTarget::GasFractions:
        return GAS_FRACTIONS;
    case ReferenceTarget::InitialGas:
        return INITIAL_GAS;
    case ReferenceTarget::DesignVariable:
        return DESIGN_VARIABLE;
    case ReferenceTarget::DesignEquation:
        return DESIGN_EQUATION;
    case ReferenceTarget::DesignResponse:
        return DESIGN_RESPONSE;
    case ReferenceTarget::DesignConstraintSet:
        return DESIGN_CONSTRAINT_SET;
    case ReferenceTarget::PropertyRelation:
        return PROPERTY_RELATION;
    case ReferenceTarget::MaterialRelation:
        return MATERIAL_RELATION;
    case ReferenceTarget::ConnectivityRelation:
        return CONNECTIVITY_RELATION;
    case ReferenceTarget::AeroList:
        return AERO_LIST;
    case ReferenceTarget::AeroFactor:
        return AERO_FACTOR;
    case ReferenceTarget::AeroFlutterFactor:
        return FLUTTER_FACTOR;
    case ReferenceTarget::AeroControlSurface:
        return AERO_CONTROL_SURFACE;
    case ReferenceTarget::AeroTrimSet:
        return TRIM_SET;
    case ReferenceTarget::AeroFlutterSet:
        return FLUTTER_SET;
    case ReferenceTarget::AeroGustSet:
        return GUST_SET;
    case ReferenceTarget::DivergenceSet:
        return DIVERGENCE_SET;
    case ReferenceTarget::ControlSchedule:
        return CONTROL_SCHEDULE;
    case ReferenceTarget::ControlVector:
        return CONTROL_VECTOR;
    case ReferenceTarget::AeroNode:
        return AERO_NODE;
    case ReferenceTarget::AeroScale:
        return AERO_SCALE;
    case ReferenceTarget::Superelement:
        return SUPERELEMENT;
    case ReferenceTarget::MassCombination:
        return MASS_COMBINATION;
    case ReferenceTarget::AeroSpline:
        return AERO_SPLINE;
    case ReferenceTarget::HybridDampingRotorOptionsSpecification:
        return DAMPING_SPECIFICATION;
    case ReferenceTarget::HybridDamping:
        return HYBRID_DAMPING;
    case ReferenceTarget::ElementList:
        return ELEMENT_LIST;
    case ReferenceTarget::ModuleMove:
        return MODULE_MOVE;
    case ReferenceTarget::MultiDimensionalTable:
        return MULTI_DIMENSIONAL_TABLE;
    case ReferenceTarget::FatigueTable:
        return FATIGUE_TABLE;
    case ReferenceTarget::ImperfectionCase:
        return IMPERFECTION_CASE;
    case ReferenceTarget::ImperfectionGeometry:
        return IMPERFECTION_GEOMETRY;
    case ReferenceTarget::GeometricBoundary:
        return GEOMETRIC_BOUNDARY;
    case ReferenceTarget::AdaptivityCriterion:
        return ADAPTIVITY_CRITERION;
    case ReferenceTarget::AdaptivitySet:
        return ADAPTIVITY_SET;
    case ReferenceTarget::Rotor:
        return ROTOR;
    case ReferenceTarget::DiscreteValueSet:
        return DISCRETE_VALUE_SET;
    case ReferenceTarget::UserDefinedName:
        return USER_DEFINED_NAME;
    case ReferenceTarget::FatigueProperty:
        return FATIGUE_PROPERTY;
    case ReferenceTarget::FatigueLoad:
        return FATIGUE_LOAD;
    case ReferenceTarget::FatigueEvent:
        return FATIGUE_EVENT;
    case ReferenceTarget::EulerInitialization:
        return EULER_INITIALIZATION;
    case ReferenceTarget::EulerInitialRegions:
        return EULER_INITIAL_REGIONS;
    case ReferenceTarget::InitialValueSet:
        return INITIAL_VALUE_SET;
    case ReferenceTarget::HeatLossSet:
        return HEAT_LOSS_SET;
    case ReferenceTarget::HeatTransferModel:
        return HEAT_TRANSFER_MODEL;
    case ReferenceTarget::CosimulationGroup:
        return COSIMULATION_GROUP;
    case ReferenceTarget::CosimulationInput:
        return COSIMULATION_INPUT;
    case ReferenceTarget::CosimulationOutput:
        return COSIMULATION_OUTPUT;
    case ReferenceTarget::CosimulationService:
        return COSIMULATION_SERVICE;
    case ReferenceTarget::WettedElement:
        return WETTED_ELEMENT;
    case ReferenceTarget::WettedSurface:
        return WETTED_SURFACE;
    case ReferenceTarget::BrakeProperty:
        return BRAKE_PROPERTY;
    case ReferenceTarget::BrakeMotion:
        return BRAKE_MOTION;
    case ReferenceTarget::BrakeSqueal:
        return BRAKE_SQUEAL;
    case ReferenceTarget::FrfComponent:
        return FRF_COMPONENT;
    case ReferenceTarget::BushingForceSet:
        return BUSHING_FORCE_SET;
    case ReferenceTarget::LoadStepControl:
        return LOAD_STEP_CONTROL;
    case ReferenceTarget::TemperatureGroup:
        return TEMPERATURE_GROUP;
    case ReferenceTarget::MonteCarloSet:
        return MONTE_CARLO_SET;
    case ReferenceTarget::DeactivatedElementSet:
        return DEACTIVATED_ELEMENT_SET;
    case ReferenceTarget::TransferPathSet:
        return TRANSFER_PATH_SET;
    case ReferenceTarget::IterativeSolver:
        return ITERATIVE_SOLVER;
    case ReferenceTarget::MetadataGroup:
        return METADATA_GROUP;
    case ReferenceTarget::Node:
    case ReferenceTarget::NodeInRange:
    case ReferenceTarget::ScalarOrGridPoint:
    case ReferenceTarget::Element:
    case ReferenceTarget::ElementInRange:
    case ReferenceTarget::SurfaceElement:
    case ReferenceTarget::SurfaceElementInRange:
    case ReferenceTarget::Property:
    case ReferenceTarget::PropertyInRange:
    case ReferenceTarget::Material:
    case ReferenceTarget::ThermalMaterial:
    case ReferenceTarget::Coordinate:
    case ReferenceTarget::DynamicTable:
    case ReferenceTarget::MaterialTable:
    case ReferenceTarget::StressTable:
    case ReferenceTarget::StressOrMultiDimensionalTable:
    case ReferenceTarget::HeatTable:
    case ReferenceTarget::HybridDampingRotorOptionsTable:
    case ReferenceTarget::RandomTable:
    case ReferenceTarget::Method:
    case ReferenceTarget::ExcitationDefined:
    case ReferenceTarget::AeroElement:
    case ReferenceTarget::AeroPanelOrComponent:
    case ReferenceTarget::AeroBox:
    case ReferenceTarget::AeroProperty:
    case ReferenceTarget::MassIncrement: // a part that carries a MASSID: no card (massIncrementsOf)
    case ReferenceTarget::Other:
        break;
    }
    return {};
}

std::optional<CardCategory> categoryOfSet(ReferenceTarget target) noexcept
{
    const std::span<const CardCategory> categories = categoriesOfSet(target);
    if (categories.size() != 1)
        return std::nullopt;
    return categories.front();
}

std::unordered_set<base::EntityId> setIdsOfCategory(const Model& model, CardCategory category)
{
    std::unordered_set<base::EntityId> ids;
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = model.findTable(type);
        // A card with no ID (SUPORT among the constraints, ASET among the sets) is in no set.
        if (table == nullptr || registry.entry(type).info.category != category ||
            registry.entry(type).info.idSpace == base::IdSpace::None)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                ids.insert(table->idAt(row));
        }
    }
    return ids;
}

std::vector<CardLocation> cardsOfSet(const Model& model, CardCategory category, base::EntityId id)
{
    std::vector<CardLocation> cards;
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = model.findTable(type);
        if (table == nullptr || registry.entry(type).info.category != category ||
            registry.entry(type).info.idSpace == base::IdSpace::None)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row) && table->idAt(row) == id)
                cards.push_back(CardLocation{type, row});
        }
    }
    return cards;
}

std::optional<CardLocation> resolveReference(const Model& model, ReferenceTarget target, base::EntityId id)
{
    if (const std::span<const CardCategory> categories = categoriesOfSet(target); !categories.empty())
    {
        for (const CardCategory category : categories)
        {
            const std::vector<CardLocation> cards = cardsOfSet(model, category, id);
            if (!cards.empty())
                return cards.front();
        }
        if (target == ReferenceTarget::ExcitationSet)
        {
            const auto defined = definedExcitations(model);
            if (const auto found = defined.find(id); found != defined.end())
                return found->second;
        }
        return std::nullopt;
    }
    // A box is found by the range of the CAERO card that has it.
    if (target == ReferenceTarget::AeroBox)
    {
        const AeroBoxRanges boxes(model);
        const AeroBoxRange* owner = boxes.owner(id);
        return owner != nullptr ? std::optional<CardLocation>(owner->location) : std::nullopt;
    }
    const auto space = spaceOf(target);
    if (!space)
        return std::nullopt;
    const std::optional<CardLocation> found = model.locate(*space, id);
    // SDAMPING selects a TABDMP1, or in direct frequency response a TABLEDi that holds a viscoelastic material.
    if (!found && target == ReferenceTarget::HybridDampingRotorOptionsTable)
        return model.locate(base::IdSpace::DynamicTable, id);
    // The material of a property is a MAT1, MAT2, ... for a structural analysis and a MAT4 or MAT5 for a heat transfer
    // analysis (the same PSOLID serves both).
    if (!found && target == ReferenceTarget::Material)
        return model.locate(base::IdSpace::ThermalMaterial, id);
    // A table of a material law (the T(RHO) of a MATTUSR) is a TABLEMi, or a TABL3Di of the SOL 400 materials; the FID
    // of a MATEP a TABLES1 or TABLEST, or a TABL3Di.
    if (!found &&
        (target == ReferenceTarget::MaterialTable || target == ReferenceTarget::StressOrMultiDimensionalTable))
    {
        const std::vector<CardLocation> multiDimensional = cardsOfSet(model, CardCategory::MultiDimensionalTable, id);
        if (!multiDimensional.empty())
            return multiDimensional.front();
    }
    // format reference SPLINE4: the CAERO field is the aerodynamic component that defines the interpolation surface: a
    // CAERO panel, or the component number of the AEQUAD4 and AETRIA3 elements of an aerodynamic mesh.
    if (!found && target == ReferenceTarget::AeroPanelOrComponent)
        return aeroMeshComponent(model, id);
    return found;
}

ReferenceResolver::ReferenceResolver(const Model& model) : m_model(model) {}

ReferenceResolver::~ReferenceResolver() = default;

const ReferenceResolver::FirstCards& ReferenceResolver::setsOf(CardCategory category)
{
    const auto known = m_sets.find(category);
    if (known != m_sets.end())
        return known->second;
    // the cards in table then row order, as cardsOfSet lists them: the first of a set ID is the one it gives first
    FirstCards& firsts = m_sets[category];
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = m_model.findTable(type);
        if (table == nullptr || registry.entry(type).info.category != category ||
            registry.entry(type).info.idSpace == base::IdSpace::None)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                firsts.emplace(table->idAt(row), CardLocation{type, row});
        }
    }
    return firsts;
}

std::optional<CardLocation> ReferenceResolver::resolve(ReferenceTarget target, base::EntityId id)
{
    if (const std::span<const CardCategory> categories = categoriesOfSet(target); !categories.empty())
    {
        for (const CardCategory category : categories)
        {
            const FirstCards& firsts = setsOf(category);
            if (const auto found = firsts.find(id); found != firsts.end())
                return found->second;
        }
        if (target == ReferenceTarget::ExcitationSet)
        {
            if (!m_excitations)
                m_excitations = definedExcitations(m_model);
            if (const auto found = m_excitations->find(id); found != m_excitations->end())
                return found->second;
        }
        return std::nullopt;
    }
    if (target == ReferenceTarget::AeroBox)
    {
        if (!m_boxes)
            m_boxes = std::make_unique<const AeroBoxRanges>(m_model);
        const AeroBoxRange* owner = m_boxes->owner(id);
        return owner != nullptr ? std::optional<CardLocation>(owner->location) : std::nullopt;
    }
    return resolveReference(m_model, target, id);
}

std::vector<UnresolvedReference> unresolvedAt(Model& model, CardLocation location)
{
    base::DiagnosticSink sink;
    // the set IDs the model keeps while cards are added one by one: a LOAD added makes no pass over every load card
    Checker checker(model, sink, base::Severity::Error, "model.unresolved-reference", true);
    CardTable* table = model.findTable(location.table);
    if (table != nullptr && table->rows().isAlive(location.row))
    {
        checker.setCard(CardRegistry::instance().entry(location.table).info.name, table->idAt(location.row));
        table->visitReferences(location.row, checker);
    }
    // The field by its name in the code (the diagnostic names it by its keyword).
    std::vector<UnresolvedReference> found;
    const std::vector<std::string>& fields = checker.reportedFields();
    for (std::size_t index = 0; index < sink.diagnostics().size(); ++index)
        found.push_back(UnresolvedReference{fields[index], sink.diagnostics()[index].message});
    return found;
}

bool referenceExists(const Model& model, ReferenceTarget target, base::EntityId id)
{
    ExistenceIndex index(model);
    return isExemptReference(target, id) || mayPointAtNothing(target) || index.exists(target, id);
}

bool mayPointAtNothing(ReferenceTarget target) noexcept
{
    return target == ReferenceTarget::NodeInRange || target == ReferenceTarget::ElementInRange ||
           target == ReferenceTarget::SurfaceElementInRange || target == ReferenceTarget::PropertyInRange ||
           target == ReferenceTarget::ScalarOrGridPoint || target == ReferenceTarget::ExcitationDefined ||
           target == ReferenceTarget::AeroBox;
}

bool isExemptReference(ReferenceTarget target, base::EntityId id) noexcept
{
    if (id == 0)
        return true; // a blank field, or the basic coordinate system
    return target == ReferenceTarget::Coordinate && id < 0;
}

std::size_t ReferenceChecker::check(Model& model, base::DiagnosticSink& sink)
{
    Checker checker(model, sink);
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            checker.setCard(registry.entry(type).info.name, table->idAt(row));
            table->visitReferences(row, checker);
        }
    }
    // The Case Control commands that name a set of cards (SPC, LOAD, METHOD, ...).
    Checker caseChecker(model, sink, base::Severity::Warning, "model.unresolved-case-control-reference");
    model.control().caseControl.visitReferences(
        caseChecker, [&caseChecker](SubcaseKind kind, std::int64_t number) { caseChecker.setScope(kind, number); });
    return checker.unresolved() + caseChecker.unresolved();
}

void ReferenceChecker::scan(const Model& model, const std::function<void(const ReferenceUse&)>& callback)
{
    // Visiting only looks: the visitor gives back each ID as it found it, and a column never writes the value it
    // already holds, so a model that is shared by several readers is not changed.
    Model& writable = const_cast<Model&>(model);
    Scanner scanner(model, callback);
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        CardTable* table = writable.findTable(type);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            scanner.setCard(type, row);
            table->visitReferences(row, scanner);
        }
    }
    // The Case Control as it is: the non-const control() of the model would clone the sections a copy shares, and
    // change its control version (a visit that writes no ID changes nothing).
    const_cast<CaseControl&>(model.control().caseControl)
        .visitReferences(scanner,
                         [&scanner](SubcaseKind kind, std::int64_t number) { scanner.setScope(kind, number); });
}

} // namespace oor::model
