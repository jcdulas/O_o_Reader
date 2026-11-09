#include "oor/model/cards/aero/aeroMachFreqTable1/AeroMachFreqTable1Table.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_MACH_FIELD = 1;
constexpr int LAST_MACH_FIELD = 8;
constexpr int FIRST_FREQUENCY_FIELD = 9;
constexpr int LAST_FIELD = 16;

} // namespace

std::optional<AeroMachFreqTable1Card> AeroMachFreqTable1Format::read(FieldReader& reader)
{
    AeroMachFreqTable1Card card;
    card.machs = readRealList(reader, FIRST_MACH_FIELD, LAST_MACH_FIELD, "machNumbers");
    card.frequencies = readRealList(reader, FIRST_FREQUENCY_FIELD, LAST_FIELD, "reducedFreqs");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 16 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroMachFreqTable1Format::check(const AeroMachFreqTable1Card& card)
{
    if (card.machs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one Mach number ({field})", "machNumber1"};
    if (card.machs.size() > AERO_MACH_FREQ_TABLE1_MAX_VALUES)
        return CardProblem{"card.bad-value", "the card holds at most 8 Mach numbers; use several cards", "machNumbers"};
    for (const double mach : card.machs)
    {
        if (const auto problem = checkReal(mach, "machNumbers", RealRule::NonNegative))
            return problem;
    }
    if (card.frequencies.empty())
        return CardProblem{"card.missing-field", "the card needs at least one reduced frequency ({field})",
                           "reducedFreq1"};
    if (card.frequencies.size() > AERO_MACH_FREQ_TABLE1_MAX_VALUES)
        return CardProblem{"card.bad-value", "the card holds at most 8 reduced frequencies; use several cards",
                           "reducedFreqs"};
    for (const double frequency : card.frequencies)
    {
        if (const auto problem = checkReal(frequency, "reducedFreqs", RealRule::Positive))
            return problem;
    }
    return std::nullopt;
}

void AeroMachFreqTable1Format::write(const AeroMachFreqTable1Card& card, FieldWriter& writer)
{
    // The frequencies are the continuation line, which is required: it is there whatever the Mach numbers.
    writeRealList(writer, FIRST_MACH_FIELD, card.machs);
    writeRealList(writer, FIRST_FREQUENCY_FIELD, card.frequencies);
}

std::uint32_t AeroMachFreqTable1Table::append(const AeroMachFreqTable1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_machs.push(card.machs);
    m_frequencies.push(card.frequencies);
    return row;
}

AeroMachFreqTable1Card AeroMachFreqTable1Table::card(std::uint32_t row) const
{
    AeroMachFreqTable1Card card;
    const std::span<const double> machs = m_machs.get(row);
    card.machs.assign(machs.begin(), machs.end());
    const std::span<const double> frequencies = m_frequencies.get(row);
    card.frequencies.assign(frequencies.begin(), frequencies.end());
    return card;
}

void AeroMachFreqTable1Table::store(std::uint32_t row, const AeroMachFreqTable1Card& card)
{
    m_machs.set(row, card.machs);
    m_frequencies.set(row, card.frequencies);
}

void AeroMachFreqTable1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_machs.keepRows(rows);
    m_frequencies.keepRows(rows);
}

} // namespace oor::model
