#include "oor/model/cards/optimisation/designDiscreteValues/DesignDiscreteValuesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIRST_VALUE_FIELD = 2;
constexpr int FIELDS_PER_LINE = 8;
constexpr int RANGE_FIELDS = 5; // first THRU last BY increment

// How close to a whole number of increments the end of a range has to be to count as reached, in increments.
constexpr double ROUNDING = 1.0e-9;

// The number of increments from the first value of a range to the last one it reaches: first + k * increment stays on
// the right side of `last` for k = 0 to this number. A range that gets nowhere near its end in
// MAX_DISCRETE_RANGE_VALUES steps stops there, which `check` refuses.
std::size_t stepsOf(const DesignDiscreteValuesRun& run)
{
    const double steps = std::floor((run.last - run.first) / run.increment + ROUNDING);
    if (!(steps >= 0.0))
        return 0;
    return steps > static_cast<double>(MAX_DISCRETE_RANGE_VALUES) ? MAX_DISCRETE_RANGE_VALUES
                                                                  : static_cast<std::size_t>(steps);
}

} // namespace

std::vector<double> designDiscreteValuesValues(const DesignDiscreteValuesCard& card)
{
    std::vector<double> values;
    for (const DesignDiscreteValuesRun& run : card.values)
    {
        if (!run.isRange())
        {
            values.push_back(run.first);
            continue;
        }
        const std::size_t steps = stepsOf(run);
        for (std::size_t step = 0; step <= steps; ++step)
            values.push_back(run.first + static_cast<double>(step) * run.increment);
        // The last value of the range is always one of the values, even when the range is not evenly divisible by the
        // increment; a value that is within a rounding error of it is it.
        if (std::fabs(values.back() - run.last) <= ROUNDING * std::fabs(run.increment))
            values.back() = run.last;
        else
            values.push_back(run.last);
    }
    return values;
}

std::optional<DesignDiscreteValuesCard> DesignDiscreteValuesFormat::read(FieldReader& reader)
{
    DesignDiscreteValuesCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    // The next field that is not blank after `field`, or 0 (the format does not allow a blank inside a range, and the
    // reader takes one in its stride).
    const auto next = [&](int field)
    {
        for (int candidate = field + 1; candidate <= last; ++candidate)
        {
            if (!reader.isBlank(candidate))
                return candidate;
        }
        return 0;
    };
    for (int field = FIRST_VALUE_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue; // a blank in a record is padding
        DesignDiscreteValuesRun run;
        run.first = reader.requiredReal(field, "values");
        run.last = run.first;
        const int through = next(field);
        if (through != 0 && (reader.text(through) == "Through"))
        {
            const int end = next(through);
            if (end == 0)
            {
                reader.error("card.missing-field", "THRU needs the last value of the range", "through");
                break;
            }
            run.last = reader.requiredReal(end, "values");
            const int by = next(end);
            if (by == 0 || !(reader.text(by) == "By"))
            {
                reader.error("card.missing-field", "a THRU range needs BY and its increment", "stepKeyword");
                break;
            }
            const int step = next(by);
            if (step == 0)
            {
                reader.error("card.missing-field", "BY needs an increment", "stepKeyword");
                break;
            }
            run.increment = reader.requiredReal(step, "increment");
            if (run.increment == 0.0)
                reader.error("card.bad-value", "the increment of a THRU range must not be zero", "increment");
            field = step;
        }
        card.values.push_back(run);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignDiscreteValuesFormat::check(const DesignDiscreteValuesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.values.empty())
        return CardProblem{"card.missing-field", "at least one value is required", "values"};
    for (const DesignDiscreteValuesRun& run : card.values)
    {
        if (const auto problem = checkReal(run.first, "values", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(run.last, "values", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(run.increment, "increment", RealRule::Finite))
            return problem;
        if (!run.isRange())
        {
            if (run.last != run.first)
                return CardProblem{"card.bad-value",
                                   "a range needs an increment: BY " + std::string(CardProblem::FIELD), "increment"};
            continue;
        }
        // The increment goes the way the range does; a range of one value (first equal to last) takes
        // either.
        if ((run.last > run.first && run.increment < 0.0) || (run.last < run.first && run.increment > 0.0))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) +
                                   " must be positive when the range goes up and negative when it goes down",
                               "increment"};
        if (stepsOf(run) + 1 > MAX_DISCRETE_RANGE_VALUES)
            return CardProblem{"card.bad-value", "the range has more than a million values: check its increment",
                               "increment"};
    }
    return std::nullopt;
}

void DesignDiscreteValuesFormat::write(const DesignDiscreteValuesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_VALUE_FIELD;
    for (const DesignDiscreteValuesRun& run : card.values)
    {
        // A range stays on one line: one that does not fit in what is left of a line starts the next one.
        const int width = run.isRange() ? RANGE_FIELDS : 1;
        const int column = (field - 1) % FIELDS_PER_LINE; // 0 for the first field of a line
        if (column + width > FIELDS_PER_LINE)
            field += FIELDS_PER_LINE - column;
        writer.real(field++, run.first);
        if (width == RANGE_FIELDS)
        {
            writer.text(field++, "Through");
            writer.real(field++, run.last);
            writer.text(field++, "By");
            writer.real(field++, run.increment);
        }
    }
}

std::uint32_t DesignDiscreteValuesTable::append(const DesignDiscreteValuesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignDiscreteValuesTable::store(std::uint32_t row, const DesignDiscreteValuesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignDiscreteValuesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignDiscreteValuesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
