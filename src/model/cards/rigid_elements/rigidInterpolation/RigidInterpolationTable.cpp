#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIRST_GROUP_FIELD = 5;
constexpr int FIELDS_PER_LINE = 8;

bool isNonEmptyComponentSet(std::int32_t components)
{
    return components > 0 && base::isComponentSet(components);
}

// The first field of the line a part that must begin a line (UM, ALPHA) goes to, when `next` is the next free field.
int lineStartAtOrAfter(int next)
{
    const int used = next - 1;
    return used % FIELDS_PER_LINE == 0 ? next : (used / FIELDS_PER_LINE + 1) * FIELDS_PER_LINE + 1;
}

// The first field in [from, to] that holds the keyword, or 0.
int findKeyword(const FieldReader& reader, int from, int to, std::string_view keyword)
{
    for (int field = from; field <= to; ++field)
    {
        if ((reader.text(field) == keyword))
            return field;
    }
    return 0;
}

// Reads the weighted groups in fields [from, end): WT, C, then grid points up to the next real. Blank fields are
// skipped (the padding before a keyword, the end of a node list).
void readGroups(FieldReader& reader, int from, int end, RigidInterpolationCard& card)
{
    int field = from;
    while (field < end && !reader.failed())
    {
        if (reader.isBlank(field))
        {
            ++field;
            continue;
        }
        RigidInterpolationGroup group;
        group.weight = reader.requiredReal(field, "weight");
        group.components = base::sortedDigits(reader.requiredInteger(field + 1, "components"));
        field += 2;
        while (field < end && !reader.failed())
        {
            if (reader.isBlank(field))
            {
                ++field;
                continue;
            }
            const auto value = reader.integerOrReal(field, "node");
            if (!value || std::holds_alternative<double>(*value))
                break; // a real is the weight of the next group
            group.nodes.push_back(std::get<std::int32_t>(*value));
            ++field;
        }
        card.groups.push_back(std::move(group));
    }
}

} // namespace

std::optional<RigidInterpolationCard> RigidInterpolationFormat::read(FieldReader& reader)
{
    RigidInterpolationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceNode = reader.requiredInteger(3, "referenceNode");
    card.refComponents = base::sortedDigits(reader.requiredInteger(4, "referenceComponents"));

    const int last = reader.lastUsedField();
    const int umField = findKeyword(reader, FIRST_GROUP_FIELD, last, "DependentDofs");
    const int alphaField = findKeyword(reader, FIRST_GROUP_FIELD, last, "ThermalExpansion");
    if (umField != 0 && alphaField != 0 && alphaField < umField)
        reader.error("card.bad-value",
                     std::string("dependentMarker") + " must come before " + std::string("alphaCoefficient"),
                     "dependentMarker");

    int groupsEnd = last + 1;
    for (const int keywordField : {umField, alphaField})
    {
        if (keywordField != 0)
            groupsEnd = std::min(groupsEnd, keywordField);
    }
    readGroups(reader, FIRST_GROUP_FIELD, groupsEnd, card);

    // UM: pairs (GMi, CMi) from the field after the keyword up to ALPHA or the end of the card.
    if (umField != 0 && !reader.failed())
    {
        const int umEnd = alphaField > umField ? alphaField : last + 1;
        for (int field = umField + 1; field < umEnd && !reader.failed(); field += 2)
        {
            // The field after the last pair may be the ALPHA keyword: it does not belong to the pair.
            const bool hasComponents = field + 1 < umEnd;
            if (reader.isBlank(field) && (!hasComponents || reader.isBlank(field + 1)))
                continue;
            RigidInterpolationDependent dependent;
            dependent.node = reader.requiredInteger(field, "dependentNode");
            if (hasComponents)
                dependent.components = base::sortedDigits(reader.requiredInteger(field + 1, "dependentComponents"));
            else
                reader.error("card.missing-field",
                             "a grid point after " + std::string("dependentMarker") + " needs its components " +
                                 std::string("dependentComponents"),
                             "dependentComponents");
            card.dependents.push_back(dependent);
        }
    }
    if (alphaField != 0)
    {
        card.alphaCoefficient = reader.real(alphaField + 1, "alphaCoefficient", 0.0);
        card.referenceTemperature = reader.real(alphaField + 2, "referenceTemperature", 0.0);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidInterpolationFormat::check(const RigidInterpolationCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.referenceNode <= 0)
        return CardProblem{"card.bad-id", "the reference grid point must be a positive integer", "referenceNode"};
    if (!isNonEmptyComponentSet(card.refComponents))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "referenceComponents"};
    if (card.groups.empty())
        return CardProblem{"card.missing-field", "the element needs at least one weighted group", "weight1"};
    for (const RigidInterpolationGroup& group : card.groups)
    {
        if (!isNonEmptyComponentSet(group.components))
            return CardProblem{
                "card.bad-value",
                CardProblem::fieldNamed("components") + " must be a list of the digits 1 to 6, each at most once",
                "components1"};
        if (group.nodes.empty())
            return CardProblem{"card.missing-field", "a weighted group needs at least one grid point", "node1"};
        for (const base::EntityId node : group.nodes)
        {
            if (node <= 0)
                return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "node1"};
        }
    }
    for (const RigidInterpolationDependent& dependent : card.dependents)
    {
        if (dependent.node <= 0)
            return CardProblem{
                "card.bad-id",
                "a grid point ID after " + CardProblem::fieldNamed("dependentMarker") + " must be a positive integer",
                "dependentNode1"};
        if (!isNonEmptyComponentSet(dependent.components))
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("dependentComponents") +
                                   " must be a list of the digits 1 to 6, each at most once",
                               "dependentComponents1"};
    }
    return std::nullopt;
}

void RigidInterpolationFormat::write(const RigidInterpolationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(3, card.referenceNode);
    writer.integer(4, card.refComponents);
    int field = FIRST_GROUP_FIELD;
    for (const RigidInterpolationGroup& group : card.groups)
    {
        writer.real(field++, group.weight);
        writer.integer(field++, group.components);
        for (const base::EntityId node : group.nodes)
            writer.integer(field++, node);
    }
    if (!card.dependents.empty())
    {
        field = lineStartAtOrAfter(field);
        writer.text(field++, "DependentDofs");
        for (const RigidInterpolationDependent& dependent : card.dependents)
        {
            writer.integer(field++, dependent.node);
            writer.integer(field++, dependent.components);
        }
    }
    if (card.alphaCoefficient != 0.0 || card.referenceTemperature != 0.0)
    {
        field = lineStartAtOrAfter(field);
        writer.text(field++, "ThermalExpansion");
        writer.real(field++, card.alphaCoefficient);
        writer.realUnlessDefault(field, card.referenceTemperature, 0.0);
    }
}

std::uint32_t RigidInterpolationTable::append(const RigidInterpolationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_referenceNode.push(card.referenceNode);
    m_refComponents.push(card.refComponents);
    std::vector<double> weights;
    std::vector<std::int32_t> components;
    std::vector<std::int32_t> sizes;
    std::vector<base::EntityId> nodes;
    for (const RigidInterpolationGroup& group : card.groups)
    {
        weights.push_back(group.weight);
        components.push_back(group.components);
        sizes.push_back(static_cast<std::int32_t>(group.nodes.size()));
        nodes.insert(nodes.end(), group.nodes.begin(), group.nodes.end());
    }
    m_groupWeight.push(weights);
    m_groupComponents.push(components);
    m_groupSize.push(sizes);
    m_groupNodes.push(nodes);
    std::vector<base::EntityId> dependentNodes;
    std::vector<std::int32_t> dependentComponents;
    for (const RigidInterpolationDependent& dependent : card.dependents)
    {
        dependentNodes.push_back(dependent.node);
        dependentComponents.push_back(dependent.components);
    }
    m_dependentNodes.push(dependentNodes);
    m_dependentComponents.push(dependentComponents);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidInterpolationCard RigidInterpolationTable::card(std::uint32_t row) const
{
    RigidInterpolationCard card;
    card.id = m_id.get(row);
    card.referenceNode = m_referenceNode.get(row);
    card.refComponents = m_refComponents.get(row);
    const auto weights = m_groupWeight.get(row);
    const auto components = m_groupComponents.get(row);
    const auto sizes = m_groupSize.get(row);
    const auto nodes = m_groupNodes.get(row);
    std::size_t next = 0;
    for (std::size_t index = 0; index < weights.size(); ++index)
    {
        RigidInterpolationGroup group;
        group.weight = weights[index];
        group.components = components[index];
        const auto count = static_cast<std::size_t>(sizes[index]);
        group.nodes.assign(nodes.begin() + static_cast<std::ptrdiff_t>(next),
                           nodes.begin() + static_cast<std::ptrdiff_t>(next + count));
        next += count;
        card.groups.push_back(std::move(group));
    }
    const auto dependentNodes = m_dependentNodes.get(row);
    const auto dependentComponents = m_dependentComponents.get(row);
    for (std::size_t index = 0; index < dependentNodes.size(); ++index)
        card.dependents.push_back(RigidInterpolationDependent{dependentNodes[index], dependentComponents[index]});
    card.alphaCoefficient = m_alphaCoefficient.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    return card;
}

void RigidInterpolationTable::store(std::uint32_t row, const RigidInterpolationCard& card)
{
    m_id.set(row, card.id);
    m_referenceNode.set(row, card.referenceNode);
    m_refComponents.set(row, card.refComponents);
    std::vector<double> weights;
    std::vector<std::int32_t> components;
    std::vector<std::int32_t> sizes;
    std::vector<base::EntityId> nodes;
    for (const RigidInterpolationGroup& group : card.groups)
    {
        weights.push_back(group.weight);
        components.push_back(group.components);
        sizes.push_back(static_cast<std::int32_t>(group.nodes.size()));
        nodes.insert(nodes.end(), group.nodes.begin(), group.nodes.end());
    }
    m_groupWeight.set(row, weights);
    m_groupComponents.set(row, components);
    m_groupSize.set(row, sizes);
    m_groupNodes.set(row, nodes);
    std::vector<base::EntityId> dependentNodes;
    std::vector<std::int32_t> dependentComponents;
    for (const RigidInterpolationDependent& dependent : card.dependents)
    {
        dependentNodes.push_back(dependent.node);
        dependentComponents.push_back(dependent.components);
    }
    m_dependentNodes.set(row, dependentNodes);
    m_dependentComponents.set(row, dependentComponents);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidInterpolationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_referenceNode.keepRows(rows);
    m_refComponents.keepRows(rows);
    m_groupWeight.keepRows(rows);
    m_groupComponents.keepRows(rows);
    m_groupSize.keepRows(rows);
    m_groupNodes.keepRows(rows);
    m_dependentNodes.keepRows(rows);
    m_dependentComponents.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidInterpolationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_referenceNode.set(row, visitor.visit("referenceNode", ReferenceTarget::Node, m_referenceNode.get(row)));
    std::vector<base::EntityId> nodes(m_groupNodes.get(row).begin(), m_groupNodes.get(row).end());
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_groupNodes.set(row, nodes);
    std::vector<base::EntityId> dependents(m_dependentNodes.get(row).begin(), m_dependentNodes.get(row).end());
    changed = false;
    for (base::EntityId& node : dependents)
    {
        const base::EntityId updated = visitor.visit("dependentNode", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_dependentNodes.set(row, dependents);
}

} // namespace oor::model
