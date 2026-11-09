#include "oor/model/cards/explicit/porousFlowBoundary/PorousFlowBoundaryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 3; // TYPE1; each pair takes two fields and the pairs carry on over the lines

int typeFieldOf(std::size_t index)
{
    return FIRST_PAIR_FIELD + 2 * static_cast<int>(index);
}

[[nodiscard]] bool isNumberProperty(PorousFlowProperty property) noexcept
{
    return property != PorousFlowProperty::Material && property != PorousFlowProperty::Flow &&
           property != PorousFlowProperty::Method;
}

// Reads the VALUE of a pair by the property of its TYPE.
PorousFlowBoundaryEntry readValue(FieldReader& reader, PorousFlowProperty property, int field, const char* name)
{
    PorousFlowBoundaryEntry entry;
    entry.property = property;
    switch (property)
    {
    case PorousFlowProperty::Material:
        entry.material = reader.requiredInteger(field, name);
        break;
    case PorousFlowProperty::Flow:
        entry.flow = readKeyword(reader, field, name, FLOW_DIRECTION_WORDS, FlowDirection::Both);
        break;
    case PorousFlowProperty::Method:
        entry.method = readKeyword(reader, field, name, POROUS_FLOW_METHOD_WORDS, PorousFlowMethod::Velocity);
        break;
    default:
        entry.number = reader.requiredReal(field, name);
        break;
    }
    return entry;
}

} // namespace

std::optional<PorousFlowBoundaryCard> PorousFlowBoundaryFormat::read(FieldReader& reader)
{
    PorousFlowBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    if (!reader.isBlank(2))
        reader.error("card.bad-value",
                     "field 2 of a " + std::string(reader.cardName()) + " is not used and must be blank", "");
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; typeFieldOf(index) <= last; ++index)
    {
        if (index >= POROUS_FLOW_MAX_ENTRIES)
        {
            reader.error("card.bad-value",
                         "a " + std::string(reader.cardName()) + " has at most " +
                             std::to_string(POROUS_FLOW_MAX_ENTRIES) + " pairs of " + std::string("propertyKind1") +
                             " and " + std::string("propertyValue1"),
                         "");
            break;
        }
        const int typeField = typeFieldOf(index);
        const int valueField = typeField + 1;
        if (reader.isBlank(typeField) && reader.isBlank(valueField))
            continue;
        const char* typeName = POROUS_FLOW_KIND_NAMES[index];
        if (reader.isBlank(typeField))
        {
            reader.error("card.missing-field",
                         std::string(typeName) + " is required: " + keywordList(POROUS_FLOW_PROPERTY_WORDS), typeName);
            continue;
        }
        const std::optional<PorousFlowProperty> property =
            parseKeyword<PorousFlowProperty>(POROUS_FLOW_PROPERTY_WORDS, reader.text(typeField));
        if (!property)
        {
            reader.error("card.bad-value",
                         std::string(typeName) + " must be " + keywordList(POROUS_FLOW_PROPERTY_WORDS) + ", not \"" +
                             std::string(reader.text(typeField)) + "\"",
                         typeName);
            continue;
        }
        card.entries.push_back(readValue(reader, *property, valueField, POROUS_FLOW_VALUE_NAMES[index]));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PorousFlowBoundaryFormat::check(const PorousFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.entries.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one pair of " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("propertyValue1"),
                           "propertyKind1"};
    if (card.entries.size() > POROUS_FLOW_MAX_ENTRIES)
        return CardProblem{"card.bad-value",
                           "the card has at most " + std::to_string(POROUS_FLOW_MAX_ENTRIES) + " pairs of " +
                               CardProblem::fieldNamed("propertyKind1") + " and " +
                               CardProblem::fieldNamed("propertyValue1"),
                           "propertyKind1"};
    for (std::size_t index = 0; index < card.entries.size(); ++index)
    {
        const PorousFlowBoundaryEntry& entry = card.entries[index];
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (card.entries[earlier].property == entry.property)
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " repeats " +
                                       std::string(keywordText(POROUS_FLOW_PROPERTY_WORDS, entry.property)) +
                                       ", given once before",
                                   POROUS_FLOW_KIND_NAMES[index]};
        }
        if (entry.property == PorousFlowProperty::Material)
        {
            if (const auto problem = checkRequiredId(entry.material, POROUS_FLOW_VALUE_NAMES[index]))
                return problem;
        }
        else if (isNumberProperty(entry.property))
        {
            if (const auto problem = checkReal(entry.number, POROUS_FLOW_VALUE_NAMES[index], RealRule::Finite))
                return problem;
        }
    }
    return std::nullopt;
}

void PorousFlowBoundaryFormat::write(const PorousFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.entries.size(); ++index)
    {
        const PorousFlowBoundaryEntry& entry = card.entries[index];
        const int typeField = typeFieldOf(index);
        const int valueField = typeField + 1;
        writeKeyword(writer, typeField, POROUS_FLOW_PROPERTY_WORDS, entry.property);
        switch (entry.property)
        {
        case PorousFlowProperty::Material:
            writer.integer(valueField, entry.material);
            break;
        case PorousFlowProperty::Flow:
            writeKeyword(writer, valueField, FLOW_DIRECTION_WORDS, entry.flow);
            break;
        case PorousFlowProperty::Method:
            writeKeyword(writer, valueField, POROUS_FLOW_METHOD_WORDS, entry.method);
            break;
        default:
            writer.real(valueField, entry.number);
            break;
        }
    }
}

std::uint32_t PorousFlowBoundaryTable::append(const PorousFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PorousFlowBoundaryTable::store(std::uint32_t row, const PorousFlowBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PorousFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PorousFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PorousFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    PorousFlowBoundaryCard& card = m_cards.edit(row);
    for (std::size_t index = 0; index < card.entries.size() && index < POROUS_FLOW_MAX_ENTRIES; ++index)
    {
        if (card.entries[index].property == PorousFlowProperty::Material)
            visitId(visitor, POROUS_FLOW_VALUE_REFERENCES[index], ReferenceTarget::Material,
                    card.entries[index].material);
    }
}

} // namespace oor::model
