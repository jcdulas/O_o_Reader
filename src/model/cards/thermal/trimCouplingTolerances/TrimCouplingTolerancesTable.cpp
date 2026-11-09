#include "oor/model/cards/thermal/trimCouplingTolerances/TrimCouplingTolerancesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 8; // RID

} // namespace

std::optional<TrimCouplingTolerancesCard> TrimCouplingTolerancesFormat::read(FieldReader& reader)
{
    TrimCouplingTolerancesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.couplingKind =
        readRequiredKeyword<TrimCouplingTolerancesType>(reader, 2, "couplingKind", TRIM_TOLERANCES_TYPE_WORDS);
    card.relativeTolerance = reader.real(3, "relativeTolerance", TRIM_TOLERANCES_DEFAULT_RELATIVE);
    // Each tolerance after the first defaults to the one before it: stored resolved.
    card.gapTolerance1 = reader.real(4, "gapTolerance1", TRIM_TOLERANCES_DEFAULT_GAP);
    card.gapTolerance2 = reader.real(5, "gapTolerance2", card.gapTolerance1);
    card.gapTolerance3 = reader.real(6, "gapTolerance3", card.gapTolerance2);
    card.gapTolerance4 = reader.real(7, "gapTolerance4", card.gapTolerance3);
    card.regionId = reader.integer(LAST_FIELD, "regionId", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("regionId"), "regionId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TrimCouplingTolerancesFormat::check(const TrimCouplingTolerancesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.relativeTolerance, "relativeTolerance", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gapTolerance1, "gapTolerance1", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gapTolerance2, "gapTolerance2", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gapTolerance3, "gapTolerance3", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gapTolerance4, "gapTolerance4", RealRule::Positive))
        return problem;
    if (card.regionId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "regionId"};
    return std::nullopt;
}

void TrimCouplingTolerancesFormat::write(const TrimCouplingTolerancesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, TRIM_TOLERANCES_TYPE_WORDS, card.couplingKind);
    writer.realUnlessDefault(3, card.relativeTolerance, TRIM_TOLERANCES_DEFAULT_RELATIVE);
    writer.real(
        4,
        card.gapTolerance1); // GAPTOL1 and the three that default to the one before it are resolved: always written
    writer.real(5, card.gapTolerance2);
    writer.real(6, card.gapTolerance3);
    writer.real(7, card.gapTolerance4);
    writer.integerUnlessDefault(LAST_FIELD, card.regionId, 0);
}

std::uint32_t TrimCouplingTolerancesTable::append(const TrimCouplingTolerancesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_couplingKind.push(static_cast<std::uint8_t>(card.couplingKind));
    m_relativeTolerance.push(card.relativeTolerance);
    m_gapTolerance1.push(card.gapTolerance1);
    m_gapTolerance2.push(card.gapTolerance2);
    m_gapTolerance3.push(card.gapTolerance3);
    m_gapTolerance4.push(card.gapTolerance4);
    m_regionId.push(card.regionId);
    return row;
}

TrimCouplingTolerancesCard TrimCouplingTolerancesTable::card(std::uint32_t row) const
{
    TrimCouplingTolerancesCard card;
    card.id = m_id.get(row);
    card.couplingKind = static_cast<TrimCouplingTolerancesType>(m_couplingKind.get(row));
    card.relativeTolerance = m_relativeTolerance.get(row);
    card.gapTolerance1 = m_gapTolerance1.get(row);
    card.gapTolerance2 = m_gapTolerance2.get(row);
    card.gapTolerance3 = m_gapTolerance3.get(row);
    card.gapTolerance4 = m_gapTolerance4.get(row);
    card.regionId = m_regionId.get(row);
    return card;
}

void TrimCouplingTolerancesTable::store(std::uint32_t row, const TrimCouplingTolerancesCard& card)
{
    m_id.set(row, card.id);
    m_couplingKind.set(row, static_cast<std::uint8_t>(card.couplingKind));
    m_relativeTolerance.set(row, card.relativeTolerance);
    m_gapTolerance1.set(row, card.gapTolerance1);
    m_gapTolerance2.set(row, card.gapTolerance2);
    m_gapTolerance3.set(row, card.gapTolerance3);
    m_gapTolerance4.set(row, card.gapTolerance4);
    m_regionId.set(row, card.regionId);
}

void TrimCouplingTolerancesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_couplingKind.keepRows(rows);
    m_relativeTolerance.keepRows(rows);
    m_gapTolerance1.keepRows(rows);
    m_gapTolerance2.keepRows(rows);
    m_gapTolerance3.keepRows(rows);
    m_gapTolerance4.keepRows(rows);
    m_regionId.keepRows(rows);
}

} // namespace oor::model
