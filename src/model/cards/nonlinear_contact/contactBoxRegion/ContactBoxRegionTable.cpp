#include "oor/model/cards/nonlinear_contact/contactBoxRegion/ContactBoxRegionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <array>
#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

// The names of the corner fields, in the order of the card: the grid points N1 to N8, or the coordinates X1, Y1, Z1
// to Z8.
constexpr std::array<const char*, 8> CORNER_NODE_NAMES{"cornerNode1", "cornerNode2", "cornerNode3", "cornerNode4",
                                                       "cornerNode5", "cornerNode6", "cornerNode7", "cornerNode8"};
constexpr std::array<const char*, 3> COORDINATE_NAMES{"cornerX", "cornerY", "cornerZ"};

// The name of the coordinate `index` (0 for X1, 23 for Z8): the letter of the axis and the number of the corner.
std::string coordinateName(std::size_t index)
{
    return std::string(COORDINATE_NAMES[index % 3]) + std::to_string(index / 3 + 1);
}

} // namespace

std::optional<ContactBoxRegionCard> ContactBoxRegionFormat::read(FieldReader& reader)
{
    ContactBoxRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.byCoordinates = (reader.text(2) == "Coordinates");
    if (card.byCoordinates)
    {
        card.inclusionRule = reader.integer(3, "inclusionRule", 0);
        for (std::size_t index = 0; index < card.cornerCoordinates.size(); ++index)
            card.cornerCoordinates[index] =
                reader.requiredReal(4 + static_cast<int>(index), coordinateName(index).c_str());
        if (reader.lastUsedField() > 27)
            reader.error("card.bad-value",
                         "a " + std::string(reader.cardName()) + " has eight points of three coordinates", "cornerZ8");
    }
    else
    {
        card.inclusionRule = reader.integer(2, "inclusionRule", 0);
        for (std::size_t index = 0; index < card.cornerNodes.size(); ++index)
            card.cornerNodes[index] = reader.requiredInteger(3 + static_cast<int>(index), CORNER_NODE_NAMES[index]);
        if (reader.lastUsedField() > 10)
            reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has eight grid points",
                         "cornerNode8");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBoxRegionFormat::check(const ContactBoxRegionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.inclusionRule != 0 && card.inclusionRule != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "inclusionRule"};
    if (card.byCoordinates)
    {
        for (const double value : card.cornerCoordinates)
        {
            if (!std::isfinite(value))
                return CardProblem{"card.bad-value", "the coordinates of the box must be finite", "cornerX1"};
        }
        return std::nullopt;
    }
    for (std::size_t corner = 0; corner < card.cornerNodes.size(); ++corner)
    {
        if (card.cornerNodes[corner] <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", CORNER_NODE_NAMES[corner]};
    }
    return std::nullopt;
}

void ContactBoxRegionFormat::write(const ContactBoxRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.byCoordinates)
    {
        writer.text(2, "Coordinates");
        writer.integerUnlessDefault(3, card.inclusionRule, 0);
        for (std::size_t index = 0; index < card.cornerCoordinates.size(); ++index)
            writer.real(4 + static_cast<int>(index), card.cornerCoordinates[index]);
        return;
    }
    writer.integerUnlessDefault(2, card.inclusionRule, 0);
    for (std::size_t index = 0; index < card.cornerNodes.size(); ++index)
        writer.integer(3 + static_cast<int>(index), card.cornerNodes[index]);
}

std::uint32_t ContactBoxRegionTable::append(const ContactBoxRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_inclusionRule.push(card.inclusionRule);
    m_byCoordinates.push(card.byCoordinates ? 1 : 0);
    m_cornerNodes.push(card.cornerNodes);
    m_cornerCoordinates.push(card.cornerCoordinates);
    return row;
}

ContactBoxRegionCard ContactBoxRegionTable::card(std::uint32_t row) const
{
    ContactBoxRegionCard card;
    card.id = m_id.get(row);
    card.inclusionRule = m_inclusionRule.get(row);
    card.byCoordinates = m_byCoordinates.get(row) != 0;
    card.cornerNodes = m_cornerNodes.get(row);
    card.cornerCoordinates = m_cornerCoordinates.get(row);
    return card;
}

void ContactBoxRegionTable::store(std::uint32_t row, const ContactBoxRegionCard& card)
{
    m_id.set(row, card.id);
    m_inclusionRule.set(row, card.inclusionRule);
    m_byCoordinates.set(row, card.byCoordinates ? 1 : 0);
    m_cornerNodes.set(row, card.cornerNodes);
    m_cornerCoordinates.set(row, card.cornerCoordinates);
}

void ContactBoxRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_inclusionRule.keepRows(rows);
    m_byCoordinates.keepRows(rows);
    m_cornerNodes.keepRows(rows);
    m_cornerCoordinates.keepRows(rows);
}

void ContactBoxRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (m_byCoordinates.get(row) != 0)
        return;
    std::array<base::EntityId, 8> cornerNodes = m_cornerNodes.get(row);
    const std::array<base::EntityId, 8> before = cornerNodes;
    for (std::size_t corner = 0; corner < cornerNodes.size(); ++corner)
        visitContactId(cornerNodes[corner], visitor, CORNER_NODE_NAMES[corner], ReferenceTarget::Node);
    if (cornerNodes != before)
        m_cornerNodes.set(row, cornerNodes);
}

} // namespace oor::model
