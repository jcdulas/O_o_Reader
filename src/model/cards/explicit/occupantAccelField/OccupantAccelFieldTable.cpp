#include "oor/model/cards/explicit/occupantAccelField/OccupantAccelFieldTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/ExternalProgramNames.hpp"

namespace oor::model
{

namespace
{

// The first field of the names: the second line of the card.
constexpr int FIRST_NAME_FIELD = 9;

} // namespace

std::optional<OccupantAccelFieldCard> OccupantAccelFieldFormat::read(FieldReader& reader)
{
    OccupantAccelFieldCard card;
    card.setId = reader.requiredInteger(1, "setId");
    if (!reader.isBlank(2))
        reader.error("card.bad-value", "field 2 of an " + std::string(reader.cardName()) + " is blank", "");
    card.scaleFactor = reader.real(3, "scaleFactor", 1.0);
    card.directionX = reader.real(4, "directionX", 0.0);
    card.directionY = reader.real(5, "directionY", 0.0);
    card.directionZ = reader.real(6, "directionZ", 0.0);
    // The format leaves fields 7 and 8 blank, a common deck has a zero in each: a zero is accepted and not kept.
    for (const int field : {7, 8})
    {
        if (reader.real(field, "", 0.0) != 0.0)
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of an " + std::string(reader.cardName()) + " is not used",
                         "");
    }
    for (int field = FIRST_NAME_FIELD; field <= reader.lastUsedField(); ++field)
    {
        if (!reader.isBlank(field))
            card.names.push_back(std::string(reader.text(field)));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OccupantAccelFieldFormat::check(const OccupantAccelFieldCard& card)
{
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.directionX, "directionX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.directionY, "directionY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.directionZ, "directionZ", RealRule::Finite))
        return problem;
    if (card.directionX == 0.0 && card.directionY == 0.0 && card.directionZ == 0.0)
        return CardProblem{"card.bad-value", "at least one component of the direction must not be zero", "directionX"};
    if (card.names.empty())
        return CardProblem{
            "card.missing-field",
            "the card needs the name of at least one occupant segment (" + std::string(CardProblem::FIELD) + ")",
            "segmentName"};
    for (const std::string& name : card.names)
    {
        if (const auto problem = checkNameWord(name, "segmentName"))
            return problem;
    }
    return std::nullopt;
}

void OccupantAccelFieldFormat::write(const OccupantAccelFieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.realUnlessDefault(3, card.scaleFactor, 1.0);
    writer.realUnlessDefault(4, card.directionX, 0.0);
    writer.realUnlessDefault(5, card.directionY, 0.0);
    writer.realUnlessDefault(6, card.directionZ, 0.0);
    for (std::size_t index = 0; index < card.names.size(); ++index)
        writer.text(FIRST_NAME_FIELD + static_cast<int>(index), card.names[index]);
}

std::uint32_t OccupantAccelFieldTable::append(const OccupantAccelFieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void OccupantAccelFieldTable::store(std::uint32_t row, const OccupantAccelFieldCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void OccupantAccelFieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void OccupantAccelFieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

} // namespace oor::model
