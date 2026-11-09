#include "oor/model/cards/aero/aeroLabelList/AeroLabelListTable.hpp"

#include "oor/model/cards/aero/NameList.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_NAME_FIELD = 2;

} // namespace

std::optional<AeroLabelListCard> AeroLabelListFormat::read(FieldReader& reader)
{
    AeroLabelListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.names = readNameList(reader, FIRST_NAME_FIELD, "names");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroLabelListFormat::check(const AeroLabelListCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkNameList(card.names, "names");
}

void AeroLabelListFormat::write(const AeroLabelListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeNameList(writer, FIRST_NAME_FIELD, card.names);
}

std::uint32_t AeroLabelListTable::append(const AeroLabelListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroLabelListTable::store(std::uint32_t row, const AeroLabelListCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroLabelListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroLabelListTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
