#include "oor/model/cards/thermal/fluidMassElemList/FluidMassElemListTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cstdlib>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int FIRST_ELEMENT_FIELD = 2;
// The fields of a line of a card: eight, after the name.
constexpr int FIELDS_PER_LINE = 8;

// An element of the list: any integer but zero. The most negative integer has no magnitude that fits.
bool isElementItem(base::EntityId id) noexcept
{
    return id != 0 && id != std::numeric_limits<base::EntityId>::min();
}

// The elements of the list from `first` on: IDs with their signs, and "a THRU b" making a range. The rules of a range
// (the signs and the order of its ends) are the card's check, which reports them with the field THRU.
std::vector<IdRun> readElements(FieldReader& reader, int first)
{
    std::vector<IdRun> runs;
    const int last = reader.lastUsedField();
    for (int field = first; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        if ((reader.text(field) == "Through"))
        {
            if (runs.empty() || runs.back().first != runs.back().last)
            {
                reader.error("card.bad-value", "THRU must follow a single element", "through");
                break;
            }
            int next = field + 1;
            while (next <= last && reader.isBlank(next))
                ++next;
            if (next > last)
            {
                reader.error("card.missing-field", "THRU needs a last element", "through");
                break;
            }
            runs.back().last = reader.requiredInteger(next, "elemId");
            field = next;
            continue;
        }
        const base::EntityId id = reader.requiredInteger(field, "elemId");
        runs.push_back(IdRun{id, id});
    }
    return runs;
}

// True when `next` goes on from the end of `previous` on the same side: the following element, in magnitude.
bool followsOnSameSide(const IdRun& previous, const IdRun& next) noexcept
{
    return (previous.last < 0) == (next.first < 0) && std::abs(next.first) == std::abs(previous.last) + 1;
}

// The IDs and ranges from `first` on. With `compress` (the THRU option of the writer) elements that follow each other
// on one side become a range, but two of them stay two (a THRU would not be shorter). A range stays on one line and its
// THRU is never the first or the last field of a line, as the format reference asks (the writers of the other lists do
// the same). The magnitude is what the range counts, so a range of negative IDs goes from -62 to -70.
void writeElements(FieldWriter& writer, int first, const std::vector<IdRun>& runs, bool compress)
{
    std::vector<IdRun> merged;
    for (const IdRun& run : runs)
    {
        if (compress && !merged.empty() && followsOnSameSide(merged.back(), run))
            merged.back().last = run.last;
        else
            merged.push_back(run);
    }
    int field = first;
    for (const IdRun& run : merged)
    {
        const base::EntityId length = std::abs(run.last) - std::abs(run.first);
        if (length == 0)
        {
            writer.integer(field++, run.first);
        }
        else if (compress && length == 1)
        {
            writer.integer(field++, run.first);
            writer.integer(field++, run.last);
        }
        else
        {
            const int position = (field - 1) % FIELDS_PER_LINE;
            if (position + 3 > FIELDS_PER_LINE)
                field += FIELDS_PER_LINE - position;
            writer.integer(field++, run.first);
            writer.text(field++, "Through");
            writer.integer(field++, run.last);
        }
    }
}

// The visit of one element of the list: the target sees the magnitude, the card keeps the sign.
base::EntityId visitElement(ReferenceVisitor& visitor, base::EntityId id)
{
    const base::EntityId updated = visitor.visit("elemId", ReferenceTarget::ElementInRange, std::abs(id));
    return id < 0 ? -updated : updated;
}

} // namespace

std::optional<FluidMassElemListCard> FluidMassElemListFormat::read(FieldReader& reader)
{
    FluidMassElemListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elements = readElements(reader, FIRST_ELEMENT_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidMassElemListFormat::check(const FluidMassElemListCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.elements.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element", "elemId1"};
    for (const IdRun& run : card.elements)
    {
        if (!isElementItem(run.first) || !isElementItem(run.last))
            return CardProblem{"card.bad-id", "an element ID must not be zero (a minus sign gives the wetted side)",
                               "elemId1"};
        if ((run.first < 0) != (run.last < 0))
            return CardProblem{"card.bad-value", "the elements on both sides of THRU must have the same sign",
                               "through"};
        if (std::abs(run.last) < std::abs(run.first))
            return CardProblem{"card.bad-value", "the end of a THRU range must not be below its start", "through"};
    }
    return std::nullopt;
}

void FluidMassElemListFormat::write(const FluidMassElemListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeElements(writer, FIRST_ELEMENT_FIELD, card.elements, writer.compressesRanges());
}

std::uint32_t FluidMassElemListTable::append(const FluidMassElemListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FluidMassElemListTable::store(std::uint32_t row, const FluidMassElemListCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FluidMassElemListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FluidMassElemListTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FluidMassElemListTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (IdRun& run : m_cards.edit(row).elements)
    {
        if (run.first == run.last)
        {
            run.first = run.last = visitElement(visitor, run.first);
            continue;
        }
        run.first = visitElement(visitor, run.first);
        run.last = visitElement(visitor, run.last);
    }
}

} // namespace oor::model
