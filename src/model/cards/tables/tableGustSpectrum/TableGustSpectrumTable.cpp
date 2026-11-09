#include "oor/model/cards/tables/tableGustSpectrum/TableGustSpectrumTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<TableGustSpectrumCard> TableGustSpectrumFormat::read(FieldReader& reader)
{
    TableGustSpectrumCard card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = reader.requiredInteger(2, "kind");
    card.turbulenceScaleRatio = reader.requiredReal(3, "turbulenceScaleRatio");
    card.gustVelocity = reader.requiredReal(4, "gustVelocity");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("gustVelocity"),
                     "gustVelocity");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableGustSpectrumFormat::check(const TableGustSpectrumCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "id"};
    if (card.kind != TableGustSpectrumCard::VON_KARMAN && card.kind != TableGustSpectrumCard::DRYDEN)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 (von Karman) or 2 (Dryden)",
                           "kind"};
    if (!std::isfinite(card.turbulenceScaleRatio) || !std::isfinite(card.gustVelocity))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("gustVelocity") +
                               " must be finite numbers",
                           "turbulenceScaleRatio"};
    return std::nullopt;
}

void TableGustSpectrumFormat::write(const TableGustSpectrumCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.kind);
    writer.real(3, card.turbulenceScaleRatio);
    writer.real(4, card.gustVelocity);
}

std::uint32_t TableGustSpectrumTable::append(const TableGustSpectrumCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_kind.push(card.kind);
    m_turbulenceScaleRatio.push(card.turbulenceScaleRatio);
    m_gustVelocity.push(card.gustVelocity);
    return row;
}

TableGustSpectrumCard TableGustSpectrumTable::card(std::uint32_t row) const
{
    return TableGustSpectrumCard{m_id.get(row), m_kind.get(row), m_turbulenceScaleRatio.get(row),
                                 m_gustVelocity.get(row)};
}

void TableGustSpectrumTable::store(std::uint32_t row, const TableGustSpectrumCard& card)
{
    m_id.set(row, card.id);
    m_kind.set(row, card.kind);
    m_turbulenceScaleRatio.set(row, card.turbulenceScaleRatio);
    m_gustVelocity.set(row, card.gustVelocity);
}

void TableGustSpectrumTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_kind.keepRows(rows);
    m_turbulenceScaleRatio.keepRows(rows);
    m_gustVelocity.keepRows(rows);
}

} // namespace oor::model
