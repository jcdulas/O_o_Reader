#include "oor/model/cards/elements/acousticInfiniteProp/AcousticInfinitePropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6; // ZP

} // namespace

std::optional<AcousticInfinitePropCard> AcousticInfinitePropFormat::read(FieldReader& reader)
{
    AcousticInfinitePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.radialOrder = reader.requiredInteger(3, "radialOrder");
    card.poleX = reader.real(4, "poleX", 0.0);
    card.poleY = reader.real(5, "poleY", 0.0);
    card.poleZ = reader.real(6, "poleZ", 0.0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticInfinitePropFormat::check(const AcousticInfinitePropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (card.radialOrder <= 0)
        return CardProblem{
            "card.bad-value",
            "the radial interpolation order " + std::string(CardProblem::FIELD) + " must be a positive integer",
            "radialOrder"};
    if (const auto problem = checkReal(card.poleX, "poleX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.poleY, "poleY", RealRule::Finite))
        return problem;
    return checkReal(card.poleZ, "poleZ", RealRule::Finite);
}

void AcousticInfinitePropFormat::write(const AcousticInfinitePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integer(3, card.radialOrder);
    writer.realUnlessDefault(4, card.poleX, 0.0);
    writer.realUnlessDefault(5, card.poleY, 0.0);
    writer.realUnlessDefault(6, card.poleZ, 0.0);
}

std::uint32_t AcousticInfinitePropTable::append(const AcousticInfinitePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_radialOrder.push(card.radialOrder);
    m_poleX.push(card.poleX);
    m_poleY.push(card.poleY);
    m_poleZ.push(card.poleZ);
    return row;
}

AcousticInfinitePropCard AcousticInfinitePropTable::card(std::uint32_t row) const
{
    AcousticInfinitePropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.radialOrder = m_radialOrder.get(row);
    card.poleX = m_poleX.get(row);
    card.poleY = m_poleY.get(row);
    card.poleZ = m_poleZ.get(row);
    return card;
}

void AcousticInfinitePropTable::store(std::uint32_t row, const AcousticInfinitePropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_radialOrder.set(row, card.radialOrder);
    m_poleX.set(row, card.poleX);
    m_poleY.set(row, card.poleY);
    m_poleZ.set(row, card.poleZ);
}

void AcousticInfinitePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_radialOrder.keepRows(rows);
    m_poleX.keepRows(rows);
    m_poleY.keepRows(rows);
    m_poleZ.keepRows(rows);
}

void AcousticInfinitePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
