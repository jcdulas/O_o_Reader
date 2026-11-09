#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"

#include <string>
#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr std::size_t PAIRS_PER_LINE = 3;
constexpr int FIRST_INDEPENDENT_FIELD = 2;

// The field of the point of independent pair `index`: three pairs a line, the first line from field 2, the next lines
// from the third field of the line (the first one is blank).
int independentField(std::size_t index)
{
    const int line = static_cast<int>(index / PAIRS_PER_LINE);
    const int position = static_cast<int>(index % PAIRS_PER_LINE);
    return FIRST_INDEPENDENT_FIELD + FIELDS_PER_LINE * line + 2 * position;
}

// The field of "UM": the first field of the line after the independent pairs (at least one line).
int dependentMarkerField(std::size_t independentCount)
{
    const std::size_t lines = independentCount == 0 ? 1 : (independentCount + PAIRS_PER_LINE - 1) / PAIRS_PER_LINE;
    return FIELDS_PER_LINE * static_cast<int>(lines) + 1;
}

// The field of the point of dependent pair `index`, or of ALPHA when `index` is the number of pairs: three a line from
// the field after "UM", the next lines from the third field of the line.
int dependentField(int markerField, std::size_t index)
{
    const int line = static_cast<int>(index / PAIRS_PER_LINE);
    const int position = static_cast<int>(index % PAIRS_PER_LINE);
    return markerField + 1 + FIELDS_PER_LINE * line + 2 * position;
}

} // namespace

std::optional<RigidBodyMultiCard> RigidBodyMultiFormat::read(FieldReader& reader)
{
    RigidBodyMultiCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    int markerField = 0;
    for (int field = FIRST_INDEPENDENT_FIELD; field <= last; ++field)
    {
        if ((reader.text(field) == "DependentDofs"))
        {
            markerField = field;
            break;
        }
    }
    if (markerField == 0)
    {
        if (!reader.failed())
            reader.error("card.missing-field",
                         "the card needs " + std::string("dependentMarker") + " before its dependent components",
                         "dependentMarker");
        return card;
    }
    for (std::size_t index = 0; independentField(index) < markerField && !reader.failed(); ++index)
    {
        const int field = independentField(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        const std::string number = std::to_string(index + 1);
        const std::string nodeName = "independentNode" + number;
        const std::string componentName = "independentComponents" + number;
        DofPair pair;
        pair.node = reader.requiredInteger(field, nodeName.c_str());
        pair.components = base::sortedDigits(reader.requiredInteger(field + 1, componentName.c_str()));
        card.independent.push_back(pair);
    }
    for (std::size_t index = 0; dependentField(markerField, index) <= last && !reader.failed(); ++index)
    {
        const int field = dependentField(markerField, index);
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        // A TREF with no ALPHA: a real in the field after a blank point field.
        if (reader.isBlank(field))
        {
            const auto onlyTref = reader.integerOrReal(field + 1, "referenceTemperature");
            if (!reader.failed() && onlyTref && std::holds_alternative<double>(*onlyTref))
            {
                card.referenceTemperature = std::get<double>(*onlyTref);
                break;
            }
        }
        // A real where a point should be: ALPHA, and TREF in the field after it. Nothing follows them.
        const auto first = reader.integerOrReal(field, "dependentNode");
        if (reader.failed())
            break;
        if (first && std::holds_alternative<double>(*first))
        {
            card.alphaCoefficient = std::get<double>(*first);
            card.referenceTemperature = reader.real(field + 1, "referenceTemperature", 0.0);
            break;
        }
        const std::string number = std::to_string(card.dependent.size() + 1);
        const std::string nodeName = "dependentNode" + number;
        const std::string componentName = "dependentComponents" + number;
        DofPair pair;
        pair.node = reader.requiredInteger(field, nodeName.c_str());
        pair.components = base::sortedDigits(reader.requiredInteger(field + 1, componentName.c_str()));
        card.dependent.push_back(pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidBodyMultiFormat::check(const RigidBodyMultiCard& card)
{
    constexpr const char* DIGIT_LIST = " must be a list of the digits 1 to 6, each at most once";
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.independent.empty())
        return CardProblem{"card.missing-field", "the element needs at least one independent point",
                           "independentNode1"};
    if (card.independent.size() > RigidBodyMultiCard::MAX_INDEPENDENT)
        return CardProblem{"card.bad-value", "the element has at most six independent points", "independentNode1"};
    if (card.dependent.empty())
        return CardProblem{"card.missing-field", "the element needs at least one dependent point", "dependentNode1"};
    for (const DofPair& pair : card.independent)
    {
        if (pair.node <= 0)
            return CardProblem{"card.bad-id", "an independent grid point ID must be a positive integer",
                               "independentNode1"};
        if (pair.components == 0 || !base::isComponentSet(pair.components))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + DIGIT_LIST,
                               "independentComponents1"};
    }
    for (const DofPair& pair : card.dependent)
    {
        if (pair.node <= 0)
            return CardProblem{"card.bad-id", "a dependent grid point ID must be a positive integer", "dependentNode1"};
        if (pair.components == 0 || !base::isComponentSet(pair.components))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + DIGIT_LIST, "dependentComponents1"};
    }
    // a component is not both independent and dependent at a point.
    for (const DofPair& dependent : card.dependent)
    {
        for (const DofPair& independent : card.independent)
        {
            if (dependent.node == independent.node && components::overlap(dependent.components, independent.components))
                return CardProblem{"card.bad-value",
                                   "a component cannot be both independent and dependent at a grid point",
                                   "dependentComponents1"};
        }
    }
    return std::nullopt;
}

void RigidBodyMultiFormat::write(const RigidBodyMultiCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    std::size_t index = 0;
    for (const DofPair& pair : card.independent)
    {
        const int field = independentField(index++);
        writer.integer(field, pair.node);
        writer.integer(field + 1, pair.components);
    }
    const int markerField = dependentMarkerField(card.independent.size());
    writer.text(markerField, "DependentDofs");
    index = 0;
    for (const DofPair& pair : card.dependent)
    {
        const int field = dependentField(markerField, index++);
        writer.integer(field, pair.node);
        writer.integer(field + 1, pair.components);
    }
    // ALPHA takes the place of the next point, TREF the field after it. A TREF alone writes ALPHA as 0.0, so that the
    // reals at the end read back as the same two values.
    if (card.alphaCoefficient != 0.0 || card.referenceTemperature != 0.0)
    {
        const int field = dependentField(markerField, index);
        writer.real(field, card.alphaCoefficient);
        writer.realUnlessDefault(field + 1, card.referenceTemperature, 0.0);
    }
}

std::uint32_t RigidBodyMultiTable::append(const RigidBodyMultiCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.independent, nodes, components);
    m_independentNode.push(nodes);
    m_independentComponents.push(components);
    splitDofPairs(card.dependent, nodes, components);
    m_dependentNode.push(nodes);
    m_dependentComponents.push(components);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidBodyMultiCard RigidBodyMultiTable::card(std::uint32_t row) const
{
    RigidBodyMultiCard card;
    card.id = m_id.get(row);
    card.independent = joinDofPairs(m_independentNode.get(row), m_independentComponents.get(row));
    card.dependent = joinDofPairs(m_dependentNode.get(row), m_dependentComponents.get(row));
    card.alphaCoefficient = m_alphaCoefficient.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    return card;
}

void RigidBodyMultiTable::store(std::uint32_t row, const RigidBodyMultiCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.independent, nodes, components);
    m_independentNode.set(row, nodes);
    m_independentComponents.set(row, components);
    splitDofPairs(card.dependent, nodes, components);
    m_dependentNode.set(row, nodes);
    m_dependentComponents.set(row, components);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidBodyMultiTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_independentNode.keepRows(rows);
    m_independentComponents.keepRows(rows);
    m_dependentNode.keepRows(rows);
    m_dependentComponents.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidBodyMultiTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_independentNode, row, visitor, "independentNode", ReferenceTarget::Node);
    visitIdList(m_dependentNode, row, visitor, "dependentNode", ReferenceTarget::Node);
}

} // namespace oor::model
