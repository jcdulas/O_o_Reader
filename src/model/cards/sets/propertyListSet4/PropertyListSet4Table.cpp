#include "oor/model/cards/sets/propertyListSet4/PropertyListSet4Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int ENTITY_CLASS_FIELD = 2;
constexpr int TYPE_FIELD = 3;
constexpr int FIRST_ID_FIELD = 4;
// The only value of the CLASS field.
constexpr std::string_view ENTITY_CLASS_WORD = "Property";

} // namespace

std::optional<PropertyListSet4Card> PropertyListSet4Format::read(FieldReader& reader)
{
    PropertyListSet4Card card;
    card.id = reader.requiredInteger(1, "id");
    if (reader.isBlank(ENTITY_CLASS_FIELD))
        reader.error("card.missing-field", std::string("entityClass") + " is required: PROP", "entityClass");
    else if (!(reader.text(ENTITY_CLASS_FIELD) == ENTITY_CLASS_WORD))
        reader.error(
            "card.bad-value",
            std::string("entityClass") + " must be PROP, not \"" + std::string(reader.text(ENTITY_CLASS_FIELD)) + "\"",
            "entityClass");
    card.type = readRequiredKeyword<PropertyListSet4Type>(reader, TYPE_FIELD, "type", PROPERTY_LIST_SET_TYPE_WORDS);
    card.ids = readIdRuns(reader, FIRST_ID_FIELD, "ids");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PropertyListSet4Format::check(const PropertyListSet4Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkIdRuns(card.ids, "ids", IdSign::Positive);
}

void PropertyListSet4Format::write(const PropertyListSet4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(ENTITY_CLASS_FIELD, ENTITY_CLASS_WORD);
    writeKeyword(writer, TYPE_FIELD, PROPERTY_LIST_SET_TYPE_WORDS, card.type);
    // A range stays on one line and its THRU is never the first or the last field of a line (writeIdRuns).
    writeIdRuns(writer, FIRST_ID_FIELD, card.ids, writer.compressesRanges());
}

std::uint32_t PropertyListSet4Table::append(const PropertyListSet4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PropertyListSet4Table::store(std::uint32_t row, const PropertyListSet4Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PropertyListSet4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PropertyListSet4Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PropertyListSet4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The IDs are properties, of the type the card says (a rule between cards). A single ID is expected to exist; a
    // range stands for the properties that exist between its ends, as for a PCOMPF.
    visitRunList(visitor, "id", m_cards.edit(row).ids, ReferenceTarget::Property, ReferenceTarget::PropertyInRange);
}

} // namespace oor::model
