#include "DesignQuerySupport.hpp"

#include "oor/analysis/design/DesignApplication.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"
#include "oor/model/cards/optimisation/designConstrUnion/DesignConstrUnionTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/model/design/DesignUpdate.hpp"
#include "oor/model/design/DesignableCard.hpp"
#include "oor/query/providers/ModelSource.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace oor::query
{

namespace
{

// The DCONSTR sets a DESSUB or DESGLB value names: the set itself, and the sets a DCONADD of that number adds.
std::vector<base::EntityId> constraintSetsOf(const model::Model& model, base::EntityId selected)
{
    std::vector<base::EntityId> sets{selected};
    if (const model::DesignConstrUnionTable* table = model.findTable<model::DesignConstrUnionTable>())
    {
        const model::RowBook& rows = table->rows();
        for (std::uint32_t row = 0; row < rows.rowCount(); ++row)
        {
            if (!rows.isAlive(row))
                continue;
            const model::DesignConstrUnionCard card = table->card(row);
            if (card.constraintSetId == selected)
                sets.insert(sets.end(), card.sets.begin(), card.sets.end());
        }
    }
    return sets;
}

} // namespace

std::vector<base::EntityId> selectedConstraintSets(const model::Model& model, base::EntityId selected)
{
    std::set<base::EntityId> sets;
    bool isUnion = false;
    if (const model::DesignConstrUnionTable* table = model.findTable<model::DesignConstrUnionTable>())
    {
        const model::RowBook& rows = table->rows();
        for (std::uint32_t row = 0; row < rows.rowCount(); ++row)
        {
            if (!rows.isAlive(row))
                continue;
            const model::DesignConstrUnionCard card = table->card(row);
            if (card.constraintSetId != selected)
                continue;
            isUnion = true;
            sets.insert(card.sets.begin(), card.sets.end());
        }
    }
    bool isSet = false;
    if (const model::DesignConstrTable* table = model.findTable<model::DesignConstrTable>())
    {
        const model::RowBook& rows = table->rows();
        for (std::uint32_t row = 0; row < rows.rowCount() && !isSet; ++row)
            isSet = rows.isAlive(row) && table->card(row).constraintSetId == selected;
    }
    if (isSet || !isUnion)
        sets.insert(selected);
    return std::vector<base::EntityId>(sets.begin(), sets.end());
}

DesignCase readDesignCase(const model::Model& model)
{
    DesignCase result;
    const model::CaseControl& caseControl = model.control().caseControl;
    const model::CaseLineages lineages(caseControl);
    for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
    {
        if (!result.objective)
        {
            for (const char* variant : {"MinimumObjective", "MaximumObjective", ""})
            {
                const model::CaseEntry* entry = lineages.command(subcase->key, "DesignObjectiveSelection", variant);
                if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
                {
                    result.objective = static_cast<base::EntityId>(entry->value.integer);
                    result.maximise = std::string_view(variant) == "MaximumObjective";
                    break;
                }
            }
        }
        for (const char* command : {"SubcaseDesignConstrSelection", "GlobalDesignConstrSelection"})
        {
            const model::CaseEntry* entry = lineages.command(subcase->key, command);
            if (entry == nullptr || entry->value.kind != model::CaseValueKind::Integer)
                continue;
            for (const base::EntityId set : constraintSetsOf(model, static_cast<base::EntityId>(entry->value.integer)))
            {
                std::vector<std::int32_t>& subcases = result.subcasesOfSet[set];
                const auto number = static_cast<std::int32_t>(subcase->id);
                if (std::find(subcases.begin(), subcases.end(), number) == subcases.end())
                    subcases.push_back(number);
            }
        }
    }
    for (auto& entry : result.subcasesOfSet)
        std::sort(entry.second.begin(), entry.second.end());
    return result;
}

std::vector<std::vector<std::size_t>> relationsMovedBy(const model::DesignModel& design)
{
    const std::vector<model::DesignVariable>& variables = design.variables();
    std::vector<std::vector<std::size_t>> moved(variables.size());
    // The variables a variable feeds: the independent variables of the DLINK that gives it.
    const auto feeders = [&](std::size_t variable)
    {
        std::vector<std::size_t> result{variable};
        if (variables[variable].link == 0)
            return result;
        for (const model::DesignLink& link : design.links())
        {
            if (link.id != variables[variable].link)
                continue;
            for (const model::DesignTerm& term : link.terms)
            {
                if (const auto index = design.findVariable(term.designVarId))
                    result.push_back(*index);
            }
        }
        return result;
    };
    for (std::size_t relation = 0; relation < design.relations().size(); ++relation)
    {
        for (std::size_t variable = 0; variable < variables.size(); ++variable)
        {
            if (!design.relations()[relation].usesVariable(variables[variable].id))
                continue;
            for (const std::size_t feeder : feeders(variable))
            {
                if (moved[feeder].empty() || moved[feeder].back() != relation)
                    moved[feeder].push_back(relation);
            }
        }
    }
    return moved;
}

DrivenCards::DrivenCards(std::shared_ptr<model::Model> model, const model::DesignModel& design,
                         ExecutionContext& context)
    : m_model(std::move(model)),
      m_design(design),
      m_elements(listCards(*m_model, typesOfCategory(model::CardCategory::Element))),
      m_data(m_model, m_elements, context)
{
    const std::vector<std::int32_t>& properties = m_data.properties();
    for (std::size_t position = 0; position < m_elements.size(); ++position)
    {
        m_byId.emplace(m_elements.ids[position], position);
        if (properties[position] > 0)
            m_byProperty[properties[position]].push_back(position);
    }

    // The properties that use each material, through the material references of every property card.
    CardFacts facts(*m_model);
    const std::vector<std::uint16_t> propertyTypes =
        typesInSpace(model::CardCategory::Property, base::IdSpace::Property);
    const CardList propertyCards = listCards(*m_model, propertyTypes);
    for (std::size_t index = 0; index < propertyCards.size(); ++index)
    {
        for (const base::EntityId material :
             facts.references(propertyCards.location(index), model::ReferenceTarget::Material))
        {
            std::vector<base::EntityId>& users = m_propertiesOfMaterial[material];
            if (users.empty() || users.back() != propertyCards.ids[index])
                users.push_back(propertyCards.ids[index]);
        }
    }

    for (const model::DesignRelation& relation : m_design.relations())
    {
        switch (relation.targetKind)
        {
        case model::DesignTargetKind::Property:
            m_drivenProperties.insert(relation.targetId);
            break;
        case model::DesignTargetKind::Material:
            if (const auto users = m_propertiesOfMaterial.find(relation.targetId);
                users != m_propertiesOfMaterial.end())
                m_drivenProperties.insert(users->second.begin(), users->second.end());
            break;
        default:
            m_drivenElements.insert(relation.targetId);
            break;
        }
    }
}

const RelationDrive& DrivenCards::of(std::size_t relationIndex)
{
    const auto cached = m_cache.find(relationIndex);
    if (cached != m_cache.end())
        return cached->second;
    const model::DesignRelation& relation = m_design.relations()[relationIndex];
    RelationDrive drive;
    switch (relation.targetKind)
    {
    case model::DesignTargetKind::Property:
        drive.properties.push_back(relation.targetId);
        break;
    case model::DesignTargetKind::Material:
        if (const auto users = m_propertiesOfMaterial.find(relation.targetId); users != m_propertiesOfMaterial.end())
            drive.properties = users->second;
        break;
    default:
        if (const auto element = m_byId.find(relation.targetId); element != m_byId.end())
            drive.elements.push_back(element->second);
        break;
    }
    for (const base::EntityId property : drive.properties)
    {
        if (const auto elements = m_byProperty.find(property); elements != m_byProperty.end())
            drive.elements.insert(drive.elements.end(), elements->second.begin(), elements->second.end());
    }
    std::sort(drive.properties.begin(), drive.properties.end());
    drive.properties.erase(std::unique(drive.properties.begin(), drive.properties.end()), drive.properties.end());
    std::sort(drive.elements.begin(), drive.elements.end());
    drive.elements.erase(std::unique(drive.elements.begin(), drive.elements.end()), drive.elements.end());
    return m_cache.emplace(relationIndex, std::move(drive)).first->second;
}

RelationDrive DrivenCards::of(const std::vector<std::size_t>& relations)
{
    std::set<base::EntityId> properties;
    std::set<std::size_t> elements;
    for (const std::size_t relation : relations)
    {
        const RelationDrive& drive = of(relation);
        properties.insert(drive.properties.begin(), drive.properties.end());
        elements.insert(drive.elements.begin(), drive.elements.end());
    }
    RelationDrive result;
    result.properties.assign(properties.begin(), properties.end());
    result.elements.assign(elements.begin(), elements.end());
    return result;
}

bool DrivenCards::isElementDriven(std::size_t position)
{
    if (m_drivenElements.contains(m_elements.ids[position]))
        return true;
    const std::int32_t property = m_data.properties()[position];
    return property > 0 && m_drivenProperties.contains(property);
}

model::Model modelAt(const model::Model& model, const model::DesignModel& design,
                     const std::map<std::int32_t, double>& values, model::DesignLimitMode limits)
{
    model::DesignPoint point = design.initialPoint();
    for (const auto& [id, value] : values)
    {
        if (const std::optional<std::size_t> index = design.findVariable(id))
            point.values[*index] = value;
    }
    model::Model copy = model.copy();
    model::DesignApplyOptions options;
    options.limits = limits;
    options.skipUnapplicable = true;
    (void)analysis::DesignApplication::apply(copy, design, point, options);
    return copy;
}

ElementMassList massOfEach(const model::Model& model, const CardList& elements, base::ThreadPool* pool)
{
    ElementMassList result;
    result.mass.assign(elements.size(), std::numeric_limits<double>::quiet_NaN());
    result.property.assign(elements.size(), 0);
    result.material.assign(elements.size(), 0);
    std::map<std::uint16_t, std::vector<std::size_t>> positionsOfType;
    for (std::size_t position = 0; position < elements.size(); ++position)
        positionsOfType[elements.types[position]].push_back(position);
    const MassSetting setting(model, pool);
    for (const auto& [type, positions] : positionsOfType)
    {
        std::vector<std::uint32_t> rows;
        rows.reserve(positions.size());
        for (const std::size_t position : positions)
            rows.push_back(elements.rows[position]);
        const analysis::ElementMasses masses =
            analysis::MassCalculator::compute(model, type, rows, setting.options(), pool);
        for (std::size_t index = 0; index < positions.size(); ++index)
        {
            result.mass[positions[index]] = masses.mass[index];
            result.property[positions[index]] = masses.property[index];
            result.material[positions[index]] = masses.material[index];
        }
    }
    return result;
}

double massOfElements(const model::Model& model, const std::vector<model::CardLocation>& elements,
                      base::ThreadPool* pool)
{
    std::map<std::uint16_t, std::vector<std::uint32_t>> byType;
    for (const model::CardLocation& element : elements)
        byType[element.table].push_back(element.row);
    const MassSetting setting(model, pool);
    double total = 0.0;
    for (const auto& [type, rows] : byType)
    {
        const analysis::ElementMasses masses =
            analysis::MassCalculator::compute(model, type, rows, setting.options(), pool);
        for (const double mass : masses.mass)
        {
            if (std::isfinite(mass))
                total += mass;
        }
    }
    return total;
}

} // namespace oor::query
