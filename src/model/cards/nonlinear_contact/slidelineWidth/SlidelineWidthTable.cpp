#include "oor/model/cards/nonlinear_contact/slidelineWidth/SlidelineWidthTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

double defaultIncrement(double first, double last) noexcept
{
    return last >= first ? 1.0 : -1.0;
}

} // namespace

std::optional<SlidelineWidthCard> SlidelineWidthFormat::read(FieldReader& reader)
{
    SlidelineWidthCard card;
    card.segmentsId = reader.requiredInteger(1, "segmentsId");
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
    for (int field = 2; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            const int end = next(field);
            if (card.widths.empty() || card.widths.back().isRange())
            {
                reader.error("card.bad-value", "THRU must follow a single width", "through");
                break;
            }
            if (end == 0)
            {
                reader.error("card.missing-field", "THRU needs a last width", "through");
                break;
            }
            WidthRun& run = card.widths.back();
            run.last = reader.requiredReal(end, "width");
            run.increment = defaultIncrement(run.first, run.last);
            field = end;
            continue;
        }
        if ((reader.text(field) == "By"))
        {
            const int step = next(field);
            if (card.widths.empty() || !card.widths.back().isRange())
            {
                reader.error("card.bad-value", "BY must follow a THRU range", "rangeStep");
                break;
            }
            if (step == 0)
            {
                reader.error("card.missing-field", "BY needs an increment", "rangeStep");
                break;
            }
            card.widths.back().increment = reader.requiredReal(step, "increment");
            field = step;
            continue;
        }
        const double width = reader.requiredReal(field, "width");
        card.widths.push_back(WidthRun{width, width, 0.0});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SlidelineWidthFormat::check(const SlidelineWidthCard& card)
{
    if (card.segmentsId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "segmentsId"};
    if (card.widths.empty())
        return CardProblem{"card.missing-field", "the card needs at least one width", "width"};
    for (const WidthRun& run : card.widths)
    {
        if (!(run.first > 0.0) || !(run.last > 0.0) || !std::isfinite(run.first) || !std::isfinite(run.last))
            return CardProblem{"card.bad-value", "a width must be greater than zero", "width"};
        if (!run.isRange())
        {
            if (run.first != run.last)
                return CardProblem{"card.bad-value", "a single width has no end", "width"};
            continue;
        }
        if (!std::isfinite(run.increment))
            return CardProblem{"card.bad-value", "the BY increment must be a finite number", "rangeStep"};
        if ((run.increment > 0.0) != (run.last > run.first))
            return CardProblem{"card.bad-value", "the BY increment must go from the first width to the last one",
                               "rangeStep"};
    }
    return std::nullopt;
}

void SlidelineWidthFormat::write(const SlidelineWidthCard& card, FieldWriter& writer)
{
    writer.integer(1, card.segmentsId);
    int field = 2;
    for (const WidthRun& run : card.widths)
    {
        if (!run.isRange())
        {
            writer.real(field++, run.first);
            continue;
        }
        const bool writesBy = run.increment != defaultIncrement(run.first, run.last);
        const int needed = writesBy ? 5 : 3;
        if ((field - 1) % CONTACT_LINE_FIELDS + needed > CONTACT_LINE_FIELDS)
            field = firstFieldOfNextLine(field);
        writer.real(field++, run.first);
        writer.text(field++, "Through");
        writer.real(field++, run.last);
        if (writesBy)
        {
            writer.text(field++, "By");
            writer.real(field++, run.increment);
        }
    }
}

std::uint32_t SlidelineWidthTable::append(const SlidelineWidthCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_segmentsId.push(card.segmentsId);
    std::vector<std::array<double, 3>> triples;
    for (const WidthRun& run : card.widths)
        triples.push_back({run.first, run.last, run.increment});
    m_widths.push(triples);
    return row;
}

SlidelineWidthCard SlidelineWidthTable::card(std::uint32_t row) const
{
    SlidelineWidthCard card;
    card.segmentsId = m_segmentsId.get(row);
    for (const auto& triple : m_widths.get(row))
        card.widths.push_back(WidthRun{triple[0], triple[1], triple[2]});
    return card;
}

void SlidelineWidthTable::store(std::uint32_t row, const SlidelineWidthCard& card)
{
    m_segmentsId.set(row, card.segmentsId);
    std::vector<std::array<double, 3>> triples;
    for (const WidthRun& run : card.widths)
        triples.push_back({run.first, run.last, run.increment});
    m_widths.set(row, triples);
}

void SlidelineWidthTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_segmentsId.keepRows(rows);
    m_widths.keepRows(rows);
}

void SlidelineWidthTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const base::EntityId id = m_segmentsId.get(row);
    const base::EntityId updated = visitor.visit("segmentsId", ReferenceTarget::SlidelineSegments, id);
    if (updated != id)
        m_segmentsId.set(row, updated);
}

} // namespace oor::model
