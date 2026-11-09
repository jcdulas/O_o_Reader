#include "oor/model/cards/properties/barProp/BarPropTable.hpp"

namespace oor::model
{

std::optional<BarPropCard> BarPropFormat::read(FieldReader& reader)
{
    static constexpr const char* POINT_NAMES[8] = {"stressPointCy", "stressPointCz", "stressPointDy", "stressPointDz",
                                                   "stressPointEy", "stressPointEz", "stressPointFy", "stressPointFz"};

    BarPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.area = reader.real(3, "area", 0.0);
    card.bendingInertia1 = reader.real(4, "bendingInertia1", 0.0);
    card.bendingInertia2 = reader.real(5, "bendingInertia2", 0.0);
    card.torsionConstant = reader.real(6, "torsionConstant", 0.0);
    card.nonStructMass = reader.real(7, "nonStructMass", 0.0);
    // Field 8 is not used. The four stress points follow on the first continuation line.
    std::array<double, 2>* const points[4] = {&card.stressPointC, &card.stressPointD, &card.stressPointE,
                                              &card.stressPointF};
    for (int point = 0; point < 4; ++point)
    {
        for (int axis = 0; axis < 2; ++axis)
        {
            const int index = 2 * point + axis;
            (*points[point])[static_cast<std::size_t>(axis)] = reader.real(9 + index, POINT_NAMES[index], 0.0);
        }
    }
    // A blank K1 or K2 is the default (no shear flexibility), not a value: a negative one is an error, not a blank.
    if (const auto shearFactor1 = reader.optionalReal(17, "shearFactor1"))
    {
        if (*shearFactor1 < 0.0)
            reader.error("card.bad-value", std::string("shearFactor1") + " must not be negative", "shearFactor1");
        card.shearFactor1 = *shearFactor1;
    }
    if (const auto shearFactor2 = reader.optionalReal(18, "shearFactor2"))
    {
        if (*shearFactor2 < 0.0)
            reader.error("card.bad-value", std::string("shearFactor2") + " must not be negative", "shearFactor2");
        card.shearFactor2 = *shearFactor2;
    }
    card.productInertia12 = reader.real(19, "productInertia12", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BarPropFormat::check(const BarPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (card.area < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "area"};
    if (card.bendingInertia1 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "bendingInertia1"};
    if (card.bendingInertia2 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "bendingInertia2"};
    if (card.torsionConstant < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "torsionConstant"};
    if (card.productInertia12 * card.productInertia12 > card.bendingInertia1 * card.bendingInertia2)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " squared must not exceed " +
                               CardProblem::fieldNamed("bendingInertia1") + " times " +
                               CardProblem::fieldNamed("bendingInertia2"),
                           "productInertia12"};
    if (card.shearFactor1 < 0.0 && card.shearFactor1 != BarPropCard::NO_SHEAR_FACTOR)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "shearFactor1"};
    if (card.shearFactor2 < 0.0 && card.shearFactor2 != BarPropCard::NO_SHEAR_FACTOR)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "shearFactor2"};
    // K1 and K2 must be blank when the area is zero.
    if (card.area == 0.0 &&
        (card.shearFactor1 != BarPropCard::NO_SHEAR_FACTOR || card.shearFactor2 != BarPropCard::NO_SHEAR_FACTOR))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("shearFactor2") +
                               " must be blank when " + CardProblem::fieldNamed("area") + " is 0.0",
                           "shearFactor1"};
    return std::nullopt;
}

void BarPropFormat::write(const BarPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.realUnlessDefault(3, card.area, 0.0);
    writer.realUnlessDefault(4, card.bendingInertia1, 0.0);
    writer.realUnlessDefault(5, card.bendingInertia2, 0.0);
    writer.realUnlessDefault(6, card.torsionConstant, 0.0);
    writer.realUnlessDefault(7, card.nonStructMass, 0.0);
    const std::array<double, 2>* const points[4] = {&card.stressPointC, &card.stressPointD, &card.stressPointE,
                                                    &card.stressPointF};
    for (int point = 0; point < 4; ++point)
    {
        for (int axis = 0; axis < 2; ++axis)
            writer.realUnlessDefault(9 + 2 * point + axis, (*points[point])[static_cast<std::size_t>(axis)], 0.0);
    }
    if (card.shearFactor1 != BarPropCard::NO_SHEAR_FACTOR)
        writer.real(17, card.shearFactor1);
    if (card.shearFactor2 != BarPropCard::NO_SHEAR_FACTOR)
        writer.real(18, card.shearFactor2);
    writer.realUnlessDefault(19, card.productInertia12, 0.0);
}

std::uint32_t BarPropTable::append(const BarPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_area.push(card.area);
    m_bendingInertia1.push(card.bendingInertia1);
    m_bendingInertia2.push(card.bendingInertia2);
    m_torsionConstant.push(card.torsionConstant);
    m_nonStructMass.push(card.nonStructMass);
    m_stressPointC.push(card.stressPointC);
    m_stressPointD.push(card.stressPointD);
    m_stressPointE.push(card.stressPointE);
    m_stressPointF.push(card.stressPointF);
    m_shearFactor.push({card.shearFactor1, card.shearFactor2});
    m_productInertia12.push(card.productInertia12);
    return row;
}

BarPropCard BarPropTable::card(std::uint32_t row) const
{
    BarPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.area = m_area.get(row);
    card.bendingInertia1 = m_bendingInertia1.get(row);
    card.bendingInertia2 = m_bendingInertia2.get(row);
    card.torsionConstant = m_torsionConstant.get(row);
    card.nonStructMass = m_nonStructMass.get(row);
    card.stressPointC = m_stressPointC.get(row);
    card.stressPointD = m_stressPointD.get(row);
    card.stressPointE = m_stressPointE.get(row);
    card.stressPointF = m_stressPointF.get(row);
    const std::array<double, 2> shear = m_shearFactor.get(row);
    card.shearFactor1 = shear[0];
    card.shearFactor2 = shear[1];
    card.productInertia12 = m_productInertia12.get(row);
    return card;
}

void BarPropTable::store(std::uint32_t row, const BarPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_area.set(row, card.area);
    m_bendingInertia1.set(row, card.bendingInertia1);
    m_bendingInertia2.set(row, card.bendingInertia2);
    m_torsionConstant.set(row, card.torsionConstant);
    m_nonStructMass.set(row, card.nonStructMass);
    m_stressPointC.set(row, card.stressPointC);
    m_stressPointD.set(row, card.stressPointD);
    m_stressPointE.set(row, card.stressPointE);
    m_stressPointF.set(row, card.stressPointF);
    m_shearFactor.set(row, {card.shearFactor1, card.shearFactor2});
    m_productInertia12.set(row, card.productInertia12);
}

void BarPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_area.keepRows(rows);
    m_bendingInertia1.keepRows(rows);
    m_bendingInertia2.keepRows(rows);
    m_torsionConstant.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_stressPointC.keepRows(rows);
    m_stressPointD.keepRows(rows);
    m_stressPointE.keepRows(rows);
    m_stressPointF.keepRows(rows);
    m_shearFactor.keepRows(rows);
    m_productInertia12.keepRows(rows);
}

void BarPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
