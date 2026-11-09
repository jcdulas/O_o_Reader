#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionTable.hpp"

#include "oor/model/cards/aero/NameList.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LABEL_FIELD = 2;

} // namespace

std::optional<AeroComponentUnionCard> AeroComponentUnionFormat::read(FieldReader& reader)
{
    AeroComponentUnionCard card;
    card.name = std::string(reader.text(1));
    card.labels = readNameList(reader, FIRST_LABEL_FIELD, "labels");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroComponentUnionFormat::check(const AeroComponentUnionCard& card)
{
    if (const auto problem = checkNameWord(card.name, "name"))
        return problem;
    return checkNameList(card.labels, "labels");
}

void AeroComponentUnionFormat::write(const AeroComponentUnionCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writeNameList(writer, FIRST_LABEL_FIELD, card.labels);
}

std::uint32_t AeroComponentUnionTable::append(const AeroComponentUnionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void AeroComponentUnionTable::store(std::uint32_t row, const AeroComponentUnionCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void AeroComponentUnionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
