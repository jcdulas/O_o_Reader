#include "oor/model/cards/explicit/hybridInflatorFractions/HybridInflatorFractionsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8;             // fields to a line of the small format
constexpr int FIRST_STEP_FIELD = 9; // the first field of the first continuation line
constexpr std::string_view TIME_WORD = "TimeStep";

// Reads the fractions in fields `first` to `last` after the ones already in the step. A blank field ends them: a value
// after a blank one is a gap that the card could not write back.
void readFractions(FieldReader& reader, int first, int last, GasFractionStep& step, bool& gap)
{
    for (int field = first; field <= last; ++field)
    {
        if (reader.isBlank(field))
        {
            gap = true;
            continue;
        }
        if (gap)
        {
            reader.error("card.bad-value",
                         "the " + std::string("fraction") + " fields of a time must follow one another: field " +
                             std::to_string(field) + " comes after a blank one",
                         "fraction");
            return;
        }
        step.fractions.push_back(reader.requiredReal(field, "fraction"));
    }
}

} // namespace

std::optional<HybridInflatorFractionsCard> HybridInflatorFractionsFormat::read(FieldReader& reader)
{
    HybridInflatorFractionsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "basis", GAS_FRACTION_BASIS_WORDS, GasFractionBasis::Mass);
    const int last = reader.lastUsedField();
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, last);
    // Every line that starts with TIME begins the data of a time; a line that does not goes on with the time above.
    bool gap = false;
    for (int lineStart = FIRST_STEP_FIELD; lineStart <= last && !reader.failed(); lineStart += LINE)
    {
        if ((reader.text(lineStart) == TIME_WORD))
        {
            GasFractionStep step;
            step.time = reader.requiredReal(lineStart + 1, "time");
            gap = false;
            readFractions(reader, lineStart + 2, lineStart + LINE - 1, step, gap);
            card.steps.push_back(std::move(step));
        }
        else if (card.steps.empty())
        {
            reader.error("card.bad-value",
                         "the data of an " + std::string(reader.cardName()) + " begin with the word TIME in field 9",
                         "time");
        }
        else
        {
            readFractions(reader, lineStart, lineStart + LINE - 1, card.steps.back(), gap);
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HybridInflatorFractionsFormat::check(const HybridInflatorFractionsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.steps.empty())
        return CardProblem{"card.missing-field",
                           "at least one " + std::string(CardProblem::FIELD) + " with its fractions is required",
                           "time"};
    for (const GasFractionStep& step : card.steps)
    {
        if (const auto problem = checkReal(step.time, "time", RealRule::NonNegative))
            return problem;
        bool positive = false;
        for (const double fraction : step.fractions)
        {
            if (const auto problem = checkReal(fraction, "fraction", RealRule::NonNegative))
                return problem;
            positive = positive || fraction > 0.0;
        }
        if (!positive)
            return CardProblem{
                "card.bad-value",
                "at least one " + std::string(CardProblem::FIELD) + " of every time must be greater than zero",
                "fraction"};
    }
    return std::nullopt;
}

void HybridInflatorFractionsFormat::write(const HybridInflatorFractionsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, GAS_FRACTION_BASIS_WORDS, card.type, GasFractionBasis::Mass);
    int start = FIRST_STEP_FIELD;
    for (const GasFractionStep& step : card.steps)
    {
        writer.text(start, TIME_WORD);
        writer.real(start + 1, step.time);
        for (std::size_t index = 0; index < step.fractions.size(); ++index)
            writer.real(start + 2 + static_cast<int>(index), step.fractions[index]);
        // The next time begins at the start of a line.
        const int fieldCount = 2 + static_cast<int>(step.fractions.size());
        start += LINE * ((fieldCount + LINE - 1) / LINE);
    }
}

std::uint32_t HybridInflatorFractionsTable::append(const HybridInflatorFractionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HybridInflatorFractionsTable::store(std::uint32_t row, const HybridInflatorFractionsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HybridInflatorFractionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HybridInflatorFractionsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
