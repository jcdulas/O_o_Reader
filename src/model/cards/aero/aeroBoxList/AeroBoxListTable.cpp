#include "oor/model/cards/aero/aeroBoxList/AeroBoxListTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ITEM_FIELD = 2;

} // namespace

std::optional<AeroBoxListCard> AeroBoxListFormat::read(FieldReader& reader)
{
    AeroBoxListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ids = readIdRuns(reader, FIRST_ITEM_FIELD, "entries");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroBoxListFormat::check(const AeroBoxListCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkIdRuns(card.ids, "entries", IdSign::Positive);
}

void AeroBoxListFormat::write(const AeroBoxListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeIdRuns(writer, FIRST_ITEM_FIELD, card.ids, writer.compressesRanges());
}

std::uint32_t AeroBoxListTable::append(const AeroBoxListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_runs.push(toPairs(card.ids));
    return row;
}

AeroBoxListCard AeroBoxListTable::card(std::uint32_t row) const
{
    AeroBoxListCard card;
    card.id = m_id.get(row);
    card.ids = toIdRuns(m_runs.get(row));
    return card;
}

void AeroBoxListTable::store(std::uint32_t row, const AeroBoxListCard& card)
{
    m_id.set(row, card.id);
    m_runs.set(row, toPairs(card.ids));
}

void AeroBoxListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_runs.keepRows(rows);
}

void AeroBoxListTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a range are boxes, and the boxes between them follow the card that owns them.
    visitIdRuns(m_runs, row, visitor, "entries", ReferenceTarget::AeroBox, ReferenceTarget::AeroBox);
}

} // namespace oor::model
