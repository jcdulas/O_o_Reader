#include "oor/model/cards/properties/compPropIntegration/CompPropIntegrationTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LIST_FIELD = 2;               // INT is field 1, the list follows it
constexpr int LINE_SLOTS = 8;                     // fields of a line
constexpr base::EntityId MAX_PROP_ID = 9'999'999; // format reference: 0 < PID < 10000000

// The increment of a range when BY is left out.
base::EntityId defaultIncrement(base::EntityId first, base::EntityId last) noexcept
{
    return last >= first ? 1 : -1;
}

} // namespace

std::optional<CompPropIntegrationCard> CompPropIntegrationFormat::read(FieldReader& reader)
{
    CompPropIntegrationCard card;
    card.integration = reader.integer(1, "integration", 1);
    const int last = reader.lastUsedField();
    // The next field that is not blank after `field`, or 0.
    const auto next = [&](int field)
    {
        for (int candidate = field + 1; candidate <= last; ++candidate)
        {
            if (!reader.isBlank(candidate))
                return candidate;
        }
        return 0;
    };
    bool byGiven = false;
    for (int field = FIRST_LIST_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field)) // the format reference allows blank fields in the list, for readability
            continue;
        const std::string_view text = reader.text(field);
        if ((text == "All"))
        {
            if (!card.runs.empty() || next(field) != 0)
                reader.error("card.bad-value", "ALL must be the only entry of the list", "propId");
            card.all = true;
            break;
        }
        if ((text == "Through") || (text == "ToKeyword"))
        {
            const int end = next(field);
            if (card.runs.empty() || card.runs.back().isRange())
            {
                reader.error("card.bad-value", "THRU must follow a single " + std::string("propId"), "through");
                break;
            }
            if (end == 0)
            {
                reader.error("card.missing-field", "THRU needs a last " + std::string("propId"), "through");
                break;
            }
            CompPropIntegrationRun& run = card.runs.back();
            run.last = reader.requiredInteger(end, "propId");
            run.increment = defaultIncrement(run.first, run.last);
            byGiven = false;
            field = end;
            continue;
        }
        if ((text == "By"))
        {
            if (card.runs.empty() || !card.runs.back().isRange() || byGiven)
            {
                reader.error("card.bad-value", "BY must follow a THRU range", "increment");
                break;
            }
            byGiven = true;
            // N is an integer or blank: a BY with nothing after it keeps the default increment.
            if (const int step = next(field); step != 0)
            {
                card.runs.back().increment = reader.requiredInteger(step, "increment");
                field = step;
            }
            continue;
        }
        const base::EntityId propId = reader.requiredInteger(field, "propId");
        card.runs.push_back(CompPropIntegrationRun{propId, propId, 1});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CompPropIntegrationFormat::check(const CompPropIntegrationCard& card)
{
    if (card.integration < 1 || card.integration > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3", "integration"};
    if (card.all)
    {
        if (!card.runs.empty())
            return CardProblem{"card.bad-value", "ALL cannot be followed by " + std::string(CardProblem::FIELD) + "s",
                               "propId"};
        return std::nullopt;
    }
    if (card.runs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one " + std::string(CardProblem::FIELD) + ", or ALL", "propId"};
    for (const CompPropIntegrationRun& run : card.runs)
    {
        if (run.first < 1 || run.first > MAX_PROP_ID || run.last < 1 || run.last > MAX_PROP_ID)
            return CardProblem{"card.bad-id", "a " + std::string(CardProblem::FIELD) + " must be between 1 and 9999999",
                               "propId"};
        if (!run.isRange())
        {
            if (run.increment != 1)
                return CardProblem{"card.bad-value", "BY needs a THRU range", "increment"};
            continue;
        }
        if (run.increment == 0)
            return CardProblem{"card.bad-value", "the BY increment must not be zero", "increment"};
        // A range goes the way its increment says (the default is 1 up and -1 down).
        if ((run.increment > 0) != (run.last > run.first))
            return CardProblem{
                "card.bad-value",
                "the BY increment must go from the first " + CardProblem::fieldNamed("propId") + " to the last one",
                "increment"};
    }
    return std::nullopt;
}

void CompPropIntegrationFormat::write(const CompPropIntegrationCard& card, FieldWriter& writer)
{
    // INT is always written: a blank first field in front of the PIDs reads like a missing INT.
    writer.integer(1, card.integration);
    if (card.all)
    {
        writer.text(FIRST_LIST_FIELD, "All");
        return;
    }
    int field = FIRST_LIST_FIELD;
    for (const CompPropIntegrationRun& run : card.runs)
    {
        if (!run.isRange())
        {
            writer.integer(field++, run.first);
            continue;
        }
        // "first THRU last [BY n]" stays on one line, because the solver reads the THRU form field by field.
        const bool writesBy = run.increment != defaultIncrement(run.first, run.last);
        const int needed = writesBy ? 5 : 3;
        const int position = (field - 1) % LINE_SLOTS;
        if (position + needed > LINE_SLOTS)
            field += LINE_SLOTS - position;
        writer.integer(field++, run.first);
        writer.text(field++, "Through");
        writer.integer(field++, run.last);
        if (writesBy)
        {
            writer.text(field++, "By");
            writer.integer(field++, run.increment);
        }
    }
}

std::uint32_t CompPropIntegrationTable::append(const CompPropIntegrationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_integration.push(card.integration);
    m_cards.push(card);
    return row;
}

void CompPropIntegrationTable::store(std::uint32_t row, const CompPropIntegrationCard& card)
{
    m_integration.set(row, card.integration);
    m_cards.set(row, card);
}

void CompPropIntegrationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_integration.keepRows(rows);
    m_cards.keepRows(rows);
}

void CompPropIntegrationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A PID that stands alone names a PCOMP or PCOMPG; the ends of a THRU range may name nothing (the list inside the
    // range need not be continuous), so they are the "in range" target.
    for (CompPropIntegrationRun& run : m_cards.edit(row).runs)
    {
        if (!run.isRange())
        {
            run.first = run.last = visitor.visit("propId", ReferenceTarget::Property, run.first);
            continue;
        }
        run.first = visitor.visit("propId", ReferenceTarget::PropertyInRange, run.first);
        run.last = visitor.visit("propId", ReferenceTarget::PropertyInRange, run.last);
    }
}

} // namespace oor::model
