#include "oor/model/cards/constraints/geomSinglePointConstr/GeomSinglePointConstrTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <string>

namespace oor::model
{

std::optional<GeomSinglePointConstrCard> GeomSinglePointConstrFormat::read(FieldReader& reader)
{
    GeomSinglePointConstrCard card;
    card.id = reader.requiredInteger(1, "id");
    card.components = base::sortedDigits(reader.requiredInteger(2, "components"));
    card.entityType = std::string(reader.text(3));
    if (card.entityType.empty())
    {
        reader.error("card.missing-field",
                     "a " + std::string(reader.cardName()) + " card needs the kind of geometric entity", "entityType");
        return card;
    }
    card.entityId = reader.requiredInteger(4, "entityId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeomSinglePointConstrFormat::check(const GeomSinglePointConstrCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (card.components <= 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "components"};
    if (card.entityType.empty() || card.entityType.find_first_of(" ,") != std::string::npos)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a word with no blank or comma",
                           "entityType"};
    if (card.entityId <= 0)
        return CardProblem{"card.bad-id", "the entity ID must be a positive integer", "entityId"};
    return std::nullopt;
}

void GeomSinglePointConstrFormat::write(const GeomSinglePointConstrCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.components);
    writer.text(3, card.entityType);
    writer.integer(4, card.entityId);
}

std::uint32_t GeomSinglePointConstrTable::append(const GeomSinglePointConstrCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_components.push(card.components);
    m_entityType.push(card.entityType);
    m_entityId.push(card.entityId);
    return row;
}

GeomSinglePointConstrCard GeomSinglePointConstrTable::card(std::uint32_t row) const
{
    return GeomSinglePointConstrCard{m_id.get(row), m_components.get(row), m_entityType.get(row), m_entityId.get(row)};
}

void GeomSinglePointConstrTable::store(std::uint32_t row, const GeomSinglePointConstrCard& card)
{
    m_id.set(row, card.id);
    m_components.set(row, card.components);
    m_entityType.set(row, card.entityType);
    m_entityId.set(row, card.entityId);
}

void GeomSinglePointConstrTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_components.keepRows(rows);
    m_entityType.keepRows(rows);
    m_entityId.keepRows(rows);
}

void GeomSinglePointConstrTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A geometric entity (FEFACE, GMCURV, ...): not a card of the model.
    m_entityId.set(row, visitor.visit("entityId", ReferenceTarget::Other, m_entityId.get(row)));
}

} // namespace oor::model
