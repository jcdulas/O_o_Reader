#include "oor/model/cards/aero/aeroMachFreqPairs2/AeroMachFreqPairs2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 8;

// The two fields of pair `index` (counted from 0): M in the odd field, K in the even one after it.
constexpr int machField(std::size_t index) noexcept
{
    return 1 + 2 * static_cast<int>(index);
}

std::vector<std::array<double, 2>> toArrays(const std::vector<AeroMachFreqPairs2Pair>& pairs)
{
    std::vector<std::array<double, 2>> arrays;
    arrays.reserve(pairs.size());
    for (const AeroMachFreqPairs2Pair& pair : pairs)
        arrays.push_back({pair.mach, pair.frequency});
    return arrays;
}

} // namespace

std::optional<AeroMachFreqPairs2Card> AeroMachFreqPairs2Format::read(FieldReader& reader)
{
    AeroMachFreqPairs2Card card;
    for (std::size_t index = 0; index < AERO_MACH_FREQ_PAIRS2_MAX_PAIRS; ++index)
    {
        const int field = machField(index);
        // A pair with neither field is skipped; one field alone is an error.
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        AeroMachFreqPairs2Pair pair;
        pair.mach = reader.requiredReal(field, "machNumber");
        pair.frequency = reader.requiredReal(field + 1, "reducedFreq");
        card.pairs.push_back(pair);
    }
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value",
                     "an " + std::string(reader.cardName()) + " has 8 fields and no continuation line", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroMachFreqPairs2Format::check(const AeroMachFreqPairs2Card& card)
{
    if (card.pairs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one pair ({field} and {field:reducedFreq1})",
                           "machNumber1"};
    if (card.pairs.size() > AERO_MACH_FREQ_PAIRS2_MAX_PAIRS)
        return CardProblem{"card.bad-value", "the card holds at most 4 pairs; use several cards", "machNumber"};
    for (const AeroMachFreqPairs2Pair& pair : card.pairs)
    {
        if (const auto problem = checkReal(pair.mach, "machNumber", RealRule::NonNegative))
            return problem;
        if (const auto problem = checkReal(pair.frequency, "reducedFreq", RealRule::Positive))
            return problem;
    }
    return std::nullopt;
}

void AeroMachFreqPairs2Format::write(const AeroMachFreqPairs2Card& card, FieldWriter& writer)
{
    for (std::size_t index = 0; index < card.pairs.size(); ++index)
    {
        writer.real(machField(index), card.pairs[index].mach);
        writer.real(machField(index) + 1, card.pairs[index].frequency);
    }
}

std::uint32_t AeroMachFreqPairs2Table::append(const AeroMachFreqPairs2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_pairs.push(toArrays(card.pairs));
    return row;
}

AeroMachFreqPairs2Card AeroMachFreqPairs2Table::card(std::uint32_t row) const
{
    AeroMachFreqPairs2Card card;
    for (const std::array<double, 2>& pair : m_pairs.get(row))
        card.pairs.push_back(AeroMachFreqPairs2Pair{pair[0], pair[1]});
    return card;
}

void AeroMachFreqPairs2Table::store(std::uint32_t row, const AeroMachFreqPairs2Card& card)
{
    m_pairs.set(row, toArrays(card.pairs));
}

void AeroMachFreqPairs2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_pairs.keepRows(rows);
}

} // namespace oor::model
