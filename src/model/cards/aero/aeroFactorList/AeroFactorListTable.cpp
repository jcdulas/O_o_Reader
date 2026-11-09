#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_VALUE_FIELD = 2;

} // namespace

std::optional<AeroFactorListCard> AeroFactorListFormat::read(FieldReader& reader)
{
    AeroFactorListCard card;
    card.id = reader.requiredInteger(1, "id");
    card.values = readRealList(reader, FIRST_VALUE_FIELD, reader.slotCount(), "values");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroFactorListFormat::check(const AeroFactorListCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.values.empty())
        return CardProblem{"card.missing-field", "the card needs at least one number ({field})", "value1"};
    return checkRealListFinite(card.values, "values");
}

void AeroFactorListFormat::write(const AeroFactorListCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeRealList(writer, FIRST_VALUE_FIELD, card.values);
}

std::uint32_t AeroFactorListTable::append(const AeroFactorListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_values.push(card.values);
    return row;
}

AeroFactorListCard AeroFactorListTable::card(std::uint32_t row) const
{
    AeroFactorListCard card;
    card.id = m_id.get(row);
    const std::span<const double> values = m_values.get(row);
    card.values.assign(values.begin(), values.end());
    return card;
}

void AeroFactorListTable::store(std::uint32_t row, const AeroFactorListCard& card)
{
    m_id.set(row, card.id);
    m_values.set(row, card.values);
}

void AeroFactorListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_values.keepRows(rows);
}

void AeroFactorListTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The numbers are reals: an AEFACT names no other card.
}

} // namespace oor::model
