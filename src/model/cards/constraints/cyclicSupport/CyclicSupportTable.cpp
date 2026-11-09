#include "oor/model/cards/constraints/cyclicSupport/CyclicSupportTable.hpp"

#include "oor/base/text/DigitSets.hpp"

#include <string>

namespace oor::model
{

std::optional<CyclicSupportCard> CyclicSupportFormat::read(FieldReader& reader)
{
    CyclicSupportCard card;
    card.nodeId = reader.requiredInteger(1, "nodeId");
    card.components = base::sortedDigits(reader.requiredInteger(2, "components"));
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has two fields, " + std::string("nodeId") + " and " +
                         std::string("components"),
                     "components");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CyclicSupportFormat::check(const CyclicSupportCard& card)
{
    if (card.nodeId <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "nodeId"};
    // Unlike the C of an SPC1, zero is no component here: a support needs at least one degree of freedom.
    if (card.components == 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "C must be a combination of the digits 1 to 6, each at most once",
                           "components"};
    return std::nullopt;
}

void CyclicSupportFormat::write(const CyclicSupportCard& card, FieldWriter& writer)
{
    writer.integer(1, card.nodeId);
    writer.integer(2, card.components);
}

std::uint32_t CyclicSupportTable::append(const CyclicSupportCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_nodeId.push(card.nodeId);
    m_components.push(card.components);
    return row;
}

CyclicSupportCard CyclicSupportTable::card(std::uint32_t row) const
{
    return CyclicSupportCard{m_nodeId.get(row), m_components.get(row)};
}

void CyclicSupportTable::store(std::uint32_t row, const CyclicSupportCard& card)
{
    m_nodeId.set(row, card.nodeId);
    m_components.set(row, card.components);
}

void CyclicSupportTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_nodeId.keepRows(rows);
    m_components.keepRows(rows);
}

void CyclicSupportTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // GID must be a grid point, not a scalar point: Node resolves both, so telling them apart is a check
    // between cards.
    m_nodeId.set(row, visitor.visit("nodeId", ReferenceTarget::Node, m_nodeId.get(row)));
}

} // namespace oor::model
