#include "oor/model/cards/nonlinear_contact/stepControlCriteria/StepControlCriteriaTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cmath>
#include <span>
#include <string>

namespace oor::model
{

namespace
{

// The fields of a criterion, counted from the field of its word (`start`, the first field of its line): the SET3 is
// next, then the pairs (YT1, MT1), (YT2, MT2) and (YT3, MT3) on the same line, and the fourth pair is the first two
// fields of the next line.
constexpr int SET_OFFSET = 1;
constexpr int FIRST_RANGE_OFFSET = 2;
constexpr std::size_t RANGES_ON_FIRST_LINE = 3;

constexpr std::array<const char*, STEP_CONTROL_MAX_RANGES> LIMIT_NAMES{"limit1", "limit2", "limit3", "limit4"};
constexpr std::array<const char*, STEP_CONTROL_MAX_RANGES> UPTO_NAMES{"rangeEnd1", "rangeEnd2", "rangeEnd3",
                                                                      "rangeEnd4"};

// The field of the YTk of the range `index` (0 to 3) of the criterion that starts at `start`.
constexpr int limitField(int start, std::size_t index) noexcept
{
    if (index < RANGES_ON_FIRST_LINE)
        return start + FIRST_RANGE_OFFSET + 2 * static_cast<int>(index);
    return start + CONTACT_LINE_FIELDS;
}

// The first field after the MT4 of the criterion that starts at `start`.
constexpr int firstFieldAfterRanges(int start) noexcept
{
    return limitField(start, STEP_CONTROL_MAX_RANGES - 1) + 2;
}

// Reads the criterion whose word is in field `section.first`, over the lines up to `section.last`.
StepControlCriteriaCriterion readCriterion(FieldReader& reader, const ContactSection& section)
{
    StepControlCriteriaCriterion criterion;
    const int start = section.first;
    if (const auto kind = parseKeyword<LoadStepCriterion>(LOAD_STEP_CRITERION_WORDS, reader.text(start)))
        criterion.kind = *kind;

    // SET3_IDi: a SET3, or ALL, or blank (every element or grid point).
    const std::string_view setText = reader.text(start + SET_OFFSET);
    if (!setText.empty() && !(setText == "All"))
        criterion.setId = reader.requiredInteger(start + SET_OFFSET, "setId");

    bool ended = false;
    for (std::size_t index = 0; index < STEP_CONTROL_MAX_RANGES && !reader.failed(); ++index)
    {
        const int limit = limitField(start, index);
        const int upTo = limit + 1;
        // The fourth range is on the line below the criterion: when that line starts the next criterion, there is none.
        const bool hasLimit = limit <= section.last && !reader.isBlank(limit);
        const bool hasUpTo = upTo <= section.last && !reader.isBlank(upTo);
        if (!hasLimit && !hasUpTo)
        {
            ended = true;
            continue;
        }
        if (ended)
        {
            reader.error("card.bad-value", "the ranges of a criterion follow one another: a pair after a blank one",
                         LIMIT_NAMES[index]);
            break;
        }
        if (!hasLimit)
        {
            reader.error("card.missing-field",
                         std::string(UPTO_NAMES[index]) + " needs its limit " + std::string(LIMIT_NAMES[index]),
                         LIMIT_NAMES[index]);
            break;
        }
        StepControlCriteriaRange range;
        range.limit = reader.requiredReal(limit, LIMIT_NAMES[index]);
        range.upTo = reader.optionalReal(upTo, UPTO_NAMES[index]);
        criterion.ranges.push_back(range);
    }
    // Nothing else may be on the lines of the criterion: the cells the format leaves blank after the fourth range, and
    // any further line.
    for (int field = firstFieldAfterRanges(start); field <= section.last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
    }
    return criterion;
}

} // namespace

std::optional<StepControlCriteriaCard> StepControlCriteriaFormat::read(FieldReader& reader)
{
    StepControlCriteriaCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::vector<ContactSection> sections =
        splitContactSections(reader, std::span<const std::string_view>(LOAD_STEP_CRITERION_WORDS));
    // The head of the card is the first line, and any line before the first criterion goes on with it: only TID is
    // there.
    for (int field = 2; field <= sections.front().last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
    }
    for (std::size_t index = 1; index < sections.size() && !reader.failed(); ++index)
    {
        const ContactSection& section = sections[index];
        if ((section.first - 1) % CONTACT_LINE_FIELDS != 0)
        {
            reader.error("card.bad-value", "the criterion word must be the first field of its line", "criterion");
            break;
        }
        card.criteria.push_back(readCriterion(reader, section));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> StepControlCriteriaFormat::check(const StepControlCriteriaCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.criteria.empty())
        return CardProblem{"card.missing-field", "the table needs at least one criterion", "criterion"};
    for (const StepControlCriteriaCriterion& criterion : card.criteria)
    {
        if (criterion.setId < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer, or ALL",
                               "setId"};
        if (criterion.ranges.empty())
            return CardProblem{"card.missing-field", "a criterion needs the limit " + std::string(CardProblem::FIELD),
                               "limit1"};
        if (criterion.ranges.size() > STEP_CONTROL_MAX_RANGES)
            return CardProblem{"card.bad-value",
                               "a criterion has at most four ranges (" + CardProblem::fieldNamed("limit1") + " to " +
                                   std::string(CardProblem::FIELD) + ")",
                               "limit4"};
        for (std::size_t index = 0; index < criterion.ranges.size(); ++index)
        {
            const StepControlCriteriaRange& range = criterion.ranges[index];
            if (const auto problem = checkReal(range.limit, LIMIT_NAMES[index], RealRule::Finite))
                return problem;
            if (const auto problem = checkOptionalReal(range.upTo, UPTO_NAMES[index], RealRule::Finite))
                return problem;
        }
    }
    return std::nullopt;
}

void StepControlCriteriaFormat::write(const StepControlCriteriaCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int start = firstFieldOfLine(1);
    for (const StepControlCriteriaCriterion& criterion : card.criteria)
    {
        writer.text(start, keywordText(LOAD_STEP_CRITERION_WORDS, criterion.kind));
        // ALL is the default and is left blank; it is written when the defaults are asked for.
        if (criterion.setId != 0)
            writer.integer(start + SET_OFFSET, criterion.setId);
        else
            writer.textUnlessDefault(start + SET_OFFSET, "All", "All");
        for (std::size_t index = 0; index < criterion.ranges.size(); ++index)
        {
            const int limit = limitField(start, index);
            writer.real(limit, criterion.ranges[index].limit);
            if (criterion.ranges[index].upTo)
                writer.real(limit + 1, *criterion.ranges[index].upTo);
        }
        start += CONTACT_LINE_FIELDS * (criterion.ranges.size() > RANGES_ON_FIRST_LINE ? 2 : 1);
    }
}

std::uint32_t StepControlCriteriaTable::append(const StepControlCriteriaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void StepControlCriteriaTable::store(std::uint32_t row, const StepControlCriteriaCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void StepControlCriteriaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void StepControlCriteriaTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void StepControlCriteriaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (StepControlCriteriaCriterion& criterion : m_cards.edit(row).criteria)
        visitId(visitor, "setId", ReferenceTarget::Set, criterion.setId);
}

} // namespace oor::model
