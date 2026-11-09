#include "oor/model/cards/elements/crackTipProp2D/CrackTipProp2DTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7;

} // namespace

std::optional<CrackTipProp2DCard> CrackTipProp2DFormat::read(FieldReader& reader)
{
    CrackTipProp2DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.thickness = reader.requiredReal(3, "thickness");
    // The format reference gives IPLANE the default 0 (plane strain).
    card.planeCondition = reader.integer(4, "planeCondition", CrackTipProp2DCard::DEFAULT_PLANE_CONDITION);
    card.nonStructMass = reader.real(5, "nonStructMass", 0.0);
    card.gammaCoefficient = reader.real(6, "gammaCoefficient", CrackTipProp2DCard::DEFAULT_GAMMA);
    card.phiAngle = reader.real(7, "phiAngle", CrackTipProp2DCard::DEFAULT_PHI);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CrackTipProp2DFormat::check(const CrackTipProp2DCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkReal(card.thickness, "thickness", RealRule::Positive))
        return problem;
    if (card.planeCondition != 0 && card.planeCondition != 1)
        return CardProblem{"card.bad-value", "{field} must be 0 (plane strain) or 1 (plane stress)", "planeCondition"};
    if (const auto problem = checkReal(card.nonStructMass, "nonStructMass", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.gammaCoefficient, "gammaCoefficient", RealRule::Finite))
        return problem;
    return checkReal(card.phiAngle, "phiAngle", RealRule::Finite);
}

void CrackTipProp2DFormat::write(const CrackTipProp2DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.thickness);
    writer.integerUnlessDefault(4, card.planeCondition, CrackTipProp2DCard::DEFAULT_PLANE_CONDITION);
    writer.realUnlessDefault(5, card.nonStructMass, 0.0);
    writer.realUnlessDefault(6, card.gammaCoefficient, CrackTipProp2DCard::DEFAULT_GAMMA);
    writer.realUnlessDefault(7, card.phiAngle, CrackTipProp2DCard::DEFAULT_PHI);
}

std::uint32_t CrackTipProp2DTable::append(const CrackTipProp2DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_thickness.push(card.thickness);
    m_planeCondition.push(card.planeCondition);
    m_nonStructMass.push(card.nonStructMass);
    m_gammaCoefficient.push(card.gammaCoefficient);
    m_phiAngle.push(card.phiAngle);
    return row;
}

CrackTipProp2DCard CrackTipProp2DTable::card(std::uint32_t row) const
{
    CrackTipProp2DCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.thickness = m_thickness.get(row);
    card.planeCondition = m_planeCondition.get(row);
    card.nonStructMass = m_nonStructMass.get(row);
    card.gammaCoefficient = m_gammaCoefficient.get(row);
    card.phiAngle = m_phiAngle.get(row);
    return card;
}

void CrackTipProp2DTable::store(std::uint32_t row, const CrackTipProp2DCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_thickness.set(row, card.thickness);
    m_planeCondition.set(row, card.planeCondition);
    m_nonStructMass.set(row, card.nonStructMass);
    m_gammaCoefficient.set(row, card.gammaCoefficient);
    m_phiAngle.set(row, card.phiAngle);
}

void CrackTipProp2DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_thickness.keepRows(rows);
    m_planeCondition.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_gammaCoefficient.keepRows(rows);
    m_phiAngle.keepRows(rows);
}

void CrackTipProp2DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A MAT1, a MAT2 or a MAT8.
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
