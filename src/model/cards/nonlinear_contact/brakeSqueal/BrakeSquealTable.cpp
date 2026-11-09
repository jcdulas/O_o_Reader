#include "oor/model/cards/nonlinear_contact/brakeSqueal/BrakeSquealTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<BrakeSquealCard> BrakeSquealFormat::read(FieldReader& reader)
{
    BrakeSquealCard card;
    card.id = reader.requiredInteger(1, "id");
    card.loadFactor = reader.real(2, "loadFactor", 0.0);
    card.areaStiffness = reader.requiredReal(3, "areaStiffness");
    card.squealOnly = readKeyword(reader, 6, "squealOnly", BRAKE_SQUEAL_ONLY_WORDS, BrakeSquealOnly::Yes);
    card.axisX = reader.optionalReal(9, "axisX");
    card.axisY = reader.optionalReal(10, "axisY");
    card.axisZ = reader.optionalReal(11, "axisZ");
    card.xCoord = reader.optionalReal(12, "xCoord");
    card.yCoord = reader.optionalReal(13, "yCoord");
    card.zCoord = reader.optionalReal(14, "zCoord");
    // The cells the format draws blank: fields 4, 5, 7 and 8 of the first line, 15 and 16 of the second, and the end.
    requireUnusedBlank(reader, {4, 5, 7, 8, 15, 16}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BrakeSquealFormat::check(const BrakeSquealCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.loadFactor, "loadFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.areaStiffness, "areaStiffness", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.axisX, "axisX", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.axisY, "axisY", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.axisZ, "axisZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.xCoord, "xCoord", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.yCoord, "yCoord", RealRule::Finite))
        return problem;
    return checkOptionalReal(card.zCoord, "zCoord", RealRule::Finite);
}

void BrakeSquealFormat::write(const BrakeSquealCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.loadFactor, 0.0);
    writer.real(3, card.areaStiffness);
    writeKeywordUnlessDefault(writer, 6, BRAKE_SQUEAL_ONLY_WORDS, card.squealOnly, BrakeSquealOnly::Yes);
    // The axis of rotation: a blank stays blank, so the second line is there only when one of its fields is.
    if (card.axisX)
        writer.real(9, *card.axisX);
    if (card.axisY)
        writer.real(10, *card.axisY);
    if (card.axisZ)
        writer.real(11, *card.axisZ);
    if (card.xCoord)
        writer.real(12, *card.xCoord);
    if (card.yCoord)
        writer.real(13, *card.yCoord);
    if (card.zCoord)
        writer.real(14, *card.zCoord);
}

std::uint32_t BrakeSquealTable::append(const BrakeSquealCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BrakeSquealTable::store(std::uint32_t row, const BrakeSquealCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BrakeSquealTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BrakeSquealTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
