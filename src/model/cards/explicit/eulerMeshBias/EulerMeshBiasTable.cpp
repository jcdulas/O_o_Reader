#include "oor/model/cards/explicit/eulerMeshBias/EulerMeshBiasTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The intervals sit on the continuation lines of the card: the first one starts at field 9, each line holds eight
// fields and only the first five of a line are used.
constexpr int FIRST_INTERVAL_FIELD = 9;
constexpr int FIELDS_PER_LINE = 8;
constexpr int USED_FIELDS_PER_INTERVAL = 5;

bool intervalLineIsBlank(const FieldReader& reader, int first)
{
    for (int field = first; field < first + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// The problem of one interval, with the number of the interval in front of the message.
std::optional<CardProblem> inInterval(std::optional<CardProblem> problem, std::size_t index)
{
    if (problem)
        problem->message = "interval " + std::to_string(index + 1) + ": " + problem->message;
    return problem;
}

// The four fields that describe the spacing of an interval, for a message.
std::string spacingFields()
{
    return CardProblem::fieldNamed("intervalGrowth") + ", " + CardProblem::fieldNamed("intervalElemCount") + ", " +
           CardProblem::fieldNamed("intervalStartSize") + " and " + CardProblem::fieldNamed("intervalEndSize");
}

// Two of GROWTH, N, DXS and DXE describe the spacing (all six pairs are allowed), or N alone.
std::optional<CardProblem> checkSpacing(const EulerMeshBiasInterval& interval)
{
    const bool given[] = {interval.growth.has_value(), interval.count.has_value(), interval.startSize.has_value(),
                          interval.endSize.has_value()};
    static constexpr const char* NAMES[] = {"intervalGrowth", "intervalElemCount", "intervalStartSize",
                                            "intervalEndSize"};
    int present = 0;
    for (const bool isGiven : given)
        present += isGiven ? 1 : 0;
    if (present == 2 || (present == 1 && interval.count))
        return std::nullopt;
    if (present < 2)
        return CardProblem{"card.missing-field",
                           "the spacing needs two of " + spacingFields() + ", or " +
                               CardProblem::fieldNamed("intervalElemCount") + " alone",
                           "intervalElemCount"};
    // Three or four given: the third one given is the first that is too much.
    int seen = 0;
    for (std::size_t index = 0; index < 4; ++index)
    {
        if (given[index] && ++seen == 3)
            return CardProblem{"card.bad-value",
                               "the spacing is given by two of " + spacingFields() + ": the others must be blank",
                               NAMES[index]};
    }
    return std::nullopt;
}

std::optional<CardProblem> checkInterval(const EulerMeshBiasInterval& interval)
{
    if (const auto problem = checkOptionalReal(interval.begin, "intervalStart", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(interval.growth, "intervalGrowth", RealRule::Positive))
        return problem;
    if (interval.count && *interval.count <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "intervalElemCount"};
    if (const auto problem = checkOptionalReal(interval.startSize, "intervalStartSize", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(interval.endSize, "intervalEndSize", RealRule::Positive))
        return problem;
    return checkSpacing(interval);
}

} // namespace

std::optional<EulerMeshBiasCard> EulerMeshBiasFormat::read(FieldReader& reader)
{
    EulerMeshBiasCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int field = 2; field < FIRST_INTERVAL_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "fields 2 to 8 of a " + std::string(reader.cardName()) + " are not used and must be blank",
                         "");
            break;
        }
    }
    const int last = reader.lastUsedField();
    for (int first = FIRST_INTERVAL_FIELD; first <= last; first += FIELDS_PER_LINE)
    {
        if (intervalLineIsBlank(reader, first))
            continue; // an empty line holds no interval
        for (int field = first + USED_FIELDS_PER_INTERVAL; field < first + FIELDS_PER_LINE; ++field)
        {
            if (!reader.isBlank(field))
            {
                reader.error("card.bad-value",
                             "only the first five fields of an interval line of a " + std::string(reader.cardName()) +
                                 " are used",
                             "");
                break;
            }
        }
        EulerMeshBiasInterval interval;
        interval.begin = reader.optionalReal(first, "intervalStart");
        interval.growth = reader.optionalReal(first + 1, "intervalGrowth");
        interval.count = reader.optionalInteger(first + 2, "intervalElemCount");
        interval.startSize = reader.optionalReal(first + 3, "intervalStartSize");
        interval.endSize = reader.optionalReal(first + 4, "intervalEndSize");
        card.intervals.push_back(interval);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerMeshBiasFormat::check(const EulerMeshBiasCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.intervals.empty())
        return CardProblem{"card.missing-field", "the card needs at least one interval", "intervalStart"};
    std::optional<double> previousBegin;
    for (std::size_t index = 0; index < card.intervals.size(); ++index)
    {
        const EulerMeshBiasInterval& interval = card.intervals[index];
        if (const auto problem = inInterval(checkInterval(interval), index))
            return problem;
        if (!interval.begin)
            continue;
        // An interval ends where the next one begins, so the begin coordinates that are given must increase.
        if (previousBegin && *interval.begin <= *previousBegin)
            return inInterval(
                CardProblem{"card.bad-value",
                            std::string(CardProblem::FIELD) + " must be greater than the previous begin coordinate",
                            "intervalStart"},
                index);
        previousBegin = interval.begin;
    }
    return std::nullopt;
}

void EulerMeshBiasFormat::write(const EulerMeshBiasCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int first = FIRST_INTERVAL_FIELD;
    for (const EulerMeshBiasInterval& interval : card.intervals)
    {
        if (interval.begin)
            writer.real(first, *interval.begin);
        if (interval.growth)
            writer.real(first + 1, *interval.growth);
        if (interval.count)
            writer.integer(first + 2, *interval.count);
        if (interval.startSize)
            writer.real(first + 3, *interval.startSize);
        if (interval.endSize)
            writer.real(first + 4, *interval.endSize);
        first += FIELDS_PER_LINE;
    }
}

std::uint32_t EulerMeshBiasTable::append(const EulerMeshBiasCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerMeshBiasTable::store(std::uint32_t row, const EulerMeshBiasCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerMeshBiasTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerMeshBiasTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
