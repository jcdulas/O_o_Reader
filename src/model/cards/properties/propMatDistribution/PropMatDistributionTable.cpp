#include "oor/model/cards/properties/propMatDistribution/PropMatDistributionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 9; // the pairs of (EID, MIDV) begin on the second line

} // namespace

std::optional<PropMatDistributionCard> PropMatDistributionFormat::read(FieldReader& reader)
{
    PropMatDistributionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readRequiredKeyword<DistributedPropertyType>(reader, 2, "type", DISTRIBUTED_PROPERTY_WORDS);
    card.propId = reader.requiredInteger(3, "propId");
    card.materialIndex = reader.integer(4, "materialIndex", 1);
    for (int field = 5; field <= 8; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is blank: the pairs of " + std::string("elemId") + " and " + std::string("elemMatId") +
                             " start in field 9",
                         "");
            break;
        }
    }
    // Four pairs to a line, from field 9. A pair with both fields blank is a gap of the layout; a pair with one of them
    // is an error (the format gives neither a default).
    for (int field = FIRST_PAIR_FIELD; field <= reader.lastUsedField(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        const base::EntityId elemId = reader.requiredInteger(field, "elemId");
        const base::EntityId elemMatId = reader.requiredInteger(field + 1, "elemMatId");
        card.entries.emplace_back(elemId, elemMatId);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PropMatDistributionFormat::check(const PropMatDistributionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (card.materialIndex < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "materialIndex"};
    std::vector<base::EntityId> elements;
    elements.reserve(card.entries.size());
    for (const auto& [elemId, elemMatId] : card.entries)
    {
        elements.push_back(elemId);
        if (const auto problem = checkRequiredId(elemMatId, "elemMatId"))
            return problem;
    }
    return checkDistributedElements(std::move(elements));
}

void PropMatDistributionFormat::write(const PropMatDistributionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, DISTRIBUTED_PROPERTY_WORDS, card.type);
    writer.integer(3, card.propId);
    writer.integerUnlessDefault(4, card.materialIndex, 1);
    int field = FIRST_PAIR_FIELD;
    for (const auto& [elemId, elemMatId] : card.entries)
    {
        writer.integer(field, elemId);
        writer.integer(field + 1, elemMatId);
        field += 2;
    }
}

std::uint32_t PropMatDistributionTable::append(const PropMatDistributionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PropMatDistributionTable::store(std::uint32_t row, const PropMatDistributionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PropMatDistributionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PropMatDistributionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PropMatDistributionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    PropMatDistributionCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    for (auto& [elemId, elemMatId] : card.entries)
    {
        visitId(visitor, "elemId", ReferenceTarget::Element, elemId);
        visitId(visitor, "elemMatId", ReferenceTarget::Material, elemMatId);
    }
}

} // namespace oor::model
