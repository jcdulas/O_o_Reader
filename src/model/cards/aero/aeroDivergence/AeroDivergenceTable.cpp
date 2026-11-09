#include "oor/model/cards/aero/aeroDivergence/AeroDivergenceTable.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int FIRST_MACH_FIELD = 3;

} // namespace

std::optional<AeroDivergenceCard> AeroDivergenceFormat::read(FieldReader& reader)
{
    AeroDivergenceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.rootCount = reader.integer(2, "rootCount", 1);
    card.machs = readRealList(reader, FIRST_MACH_FIELD, reader.slotCount(), "machNumber");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroDivergenceFormat::check(const AeroDivergenceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.rootCount < 1)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "rootCount"};
    if (card.machs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one Mach number ({field})", "machNumber1"};
    for (const double mach : card.machs)
    {
        if (const auto problem = checkReal(mach, "machNumber", RealRule::NonNegative))
            return problem;
    }
    // the Mach numbers must be distinct.
    std::vector<double> sorted = card.machs;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
        return CardProblem{"card.bad-value", "the Mach numbers of the card must be distinct", "machNumber"};
    return std::nullopt;
}

void AeroDivergenceFormat::write(const AeroDivergenceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.rootCount, 1);
    writeRealList(writer, FIRST_MACH_FIELD, card.machs);
}

std::uint32_t AeroDivergenceTable::append(const AeroDivergenceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroDivergenceTable::store(std::uint32_t row, const AeroDivergenceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroDivergenceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroDivergenceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
