#include "oor/model/cards/nonlinear_contact/brakeMotion/BrakeMotionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>
#include <utility>

namespace oor::model
{

std::optional<BrakeMotionCard> BrakeMotionFormat::read(FieldReader& reader)
{
    BrakeMotionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.velocity = reader.real(3, "velocity", 0.0);
    card.form = readRequiredKeyword<BrakeMotionForm>(reader, 9, "form", MOTION_FORM_WORDS);
    card.cosineX = reader.real(10, "cosineX", 0.0);
    card.cosineY = reader.real(11, "cosineY", 0.0);
    card.cosineZ = reader.real(12, "cosineZ", 0.0);
    card.xCoord = reader.real(13, "xCoord", 0.0);
    card.yCoord = reader.real(14, "yCoord", 0.0);
    card.zCoord = reader.real(15, "zCoord", 0.0);
    // The cells the format draws blank: fields 2 and 4 to 8 of the first line, 16 of the second, and the end.
    requireUnusedBlank(reader, {2, 4, 5, 6, 7, 8, 16}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BrakeMotionFormat::check(const BrakeMotionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.velocity, "velocity", RealRule::Finite))
        return problem;
    for (const auto& [value, name] :
         {std::pair{card.cosineX, "cosineX"}, std::pair{card.cosineY, "cosineY"}, std::pair{card.cosineZ, "cosineZ"}})
    {
        if (const auto problem = checkReal(value, name, RealRule::Finite))
            return problem;
    }
    if (card.cosineX == 0.0 && card.cosineY == 0.0 && card.cosineZ == 0.0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("cosineX") + ", " + CardProblem::fieldNamed("cosineY") + " and " +
                               CardProblem::fieldNamed("cosineZ") + " must not all be zero: they give a direction",
                           "cosineX"};
    for (const auto& [value, name] :
         {std::pair{card.xCoord, "xCoord"}, std::pair{card.yCoord, "yCoord"}, std::pair{card.zCoord, "zCoord"}})
    {
        if (const auto problem = checkReal(value, name, RealRule::Finite))
            return problem;
        // The coordinates are the center of a rotation: a translation has none.
        if (card.form == BrakeMotionForm::Trans && value != 0.0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is the center of a rotation and a TRANS has none",
                               name};
    }
    return std::nullopt;
}

void BrakeMotionFormat::write(const BrakeMotionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(3, card.velocity, 0.0);
    writeKeyword(writer, 9, MOTION_FORM_WORDS, card.form);
    // A blank real is 0.0, so a zero cosine or coordinate is written as a blank.
    writer.realUnlessDefault(10, card.cosineX, 0.0);
    writer.realUnlessDefault(11, card.cosineY, 0.0);
    writer.realUnlessDefault(12, card.cosineZ, 0.0);
    if (card.form == BrakeMotionForm::Rotat)
    {
        writer.realUnlessDefault(13, card.xCoord, 0.0);
        writer.realUnlessDefault(14, card.yCoord, 0.0);
        writer.realUnlessDefault(15, card.zCoord, 0.0);
    }
}

std::uint32_t BrakeMotionTable::append(const BrakeMotionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BrakeMotionTable::store(std::uint32_t row, const BrakeMotionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BrakeMotionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BrakeMotionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
