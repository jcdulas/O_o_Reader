#include "oor/model/cards/elements/crackTipProp3D/CrackTipProp3DTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 4;

} // namespace

std::optional<CrackTipProp3DCard> CrackTipProp3DFormat::read(FieldReader& reader)
{
    CrackTipProp3DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.gammaCoefficient = reader.real(3, "gammaCoefficient", CrackTipProp3DCard::DEFAULT_GAMMA);
    card.phiAngle = reader.real(4, "phiAngle", CrackTipProp3DCard::DEFAULT_PHI);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CrackTipProp3DFormat::check(const CrackTipProp3DCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkReal(card.gammaCoefficient, "gammaCoefficient", RealRule::Finite))
        return problem;
    return checkReal(card.phiAngle, "phiAngle", RealRule::Finite);
}

void CrackTipProp3DFormat::write(const CrackTipProp3DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.realUnlessDefault(3, card.gammaCoefficient, CrackTipProp3DCard::DEFAULT_GAMMA);
    writer.realUnlessDefault(4, card.phiAngle, CrackTipProp3DCard::DEFAULT_PHI);
}

std::uint32_t CrackTipProp3DTable::append(const CrackTipProp3DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_gammaCoefficient.push(card.gammaCoefficient);
    m_phiAngle.push(card.phiAngle);
    return row;
}

CrackTipProp3DCard CrackTipProp3DTable::card(std::uint32_t row) const
{
    CrackTipProp3DCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.gammaCoefficient = m_gammaCoefficient.get(row);
    card.phiAngle = m_phiAngle.get(row);
    return card;
}

void CrackTipProp3DTable::store(std::uint32_t row, const CrackTipProp3DCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_gammaCoefficient.set(row, card.gammaCoefficient);
    m_phiAngle.set(row, card.phiAngle);
}

void CrackTipProp3DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_gammaCoefficient.keepRows(rows);
    m_phiAngle.keepRows(rows);
}

void CrackTipProp3DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A MAT1 or a MAT9.
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
