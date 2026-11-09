#include "oor/model/cards/properties/curvedBeamProp/CurvedBeamPropTable.hpp"

#include <cmath>
#include <variant>

namespace oor::model
{

namespace
{

// Blank fields 9 and 10 and the ones after `last` of the pipe form, or the ones after the last of the section form.
void requireBlank(FieldReader& reader, int first, int last, const char* name)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "this field of a " + std::string(reader.cardName()) + " must be blank in this format", name);
            return;
        }
    }
}

double optionalOrNotGiven(FieldReader& reader, int field, const char* name)
{
    return reader.optionalReal(field, name).value_or(CurvedBeamPropCard::NOT_GIVEN);
}

} // namespace

std::optional<CurvedBeamPropCard> CurvedBeamPropFormat::read(FieldReader& reader)
{
    static constexpr const char* STRESS_NAMES[8] = {"stressPointCy", "stressPointCz", "stressPointDy", "stressPointDz",
                                                    "stressPointEy", "stressPointEz", "stressPointFy", "stressPointFz"};

    CurvedBeamPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    // The form is told by field 3: a real is the area of a general section, an integer is the FSI of a pipe.
    const auto first = reader.integerOrReal(3, "area");
    if (!first)
    {
        reader.error("card.missing-field",
                     "field 3 is the area " + std::string("area") + " (a real) or " + std::string("flexibilityOption") +
                         " (an integer)",
                     "area");
        return card;
    }
    if (const std::int32_t* flexibilityOption = std::get_if<std::int32_t>(&*first))
    {
        card.form = CurvedBeamPropForm::Pipe;
        card.flexibilityOption = *flexibilityOption;
        card.meanRadius = reader.requiredReal(4, "meanRadius");
        card.thickness = reader.requiredReal(5, "thickness");
        card.internalPressure = optionalOrNotGiven(reader, 6, "internalPressure");
        card.bendRadius = optionalOrNotGiven(reader, 7, "bendRadius");
        card.arcAngle = optionalOrNotGiven(reader, 8, "arcAngle");
        requireBlank(reader, 9, 10, "nonStructMass");
        card.nonStructMass = reader.real(11, "nonStructMass", 0.0);
        card.centroidRadialOffset = reader.real(12, "centroidRadialOffset", 0.0);
        card.centerZ = reader.real(13, "centerZ", 0.0);
        if (reader.lastUsedField() > 13)
            reader.error("card.bad-value",
                         "a " + std::string(reader.cardName()) + " with the " + std::string("flexibilityOption") +
                             " format has no field after " + std::string("centerZ"),
                         "centerZ");
    }
    else
    {
        card.form = CurvedBeamPropForm::Section;
        card.area = std::get<double>(*first);
        card.bendingInertia1 = reader.requiredReal(4, "bendingInertia1");
        card.bendingInertia2 = reader.requiredReal(5, "bendingInertia2");
        card.torsionConstant = reader.requiredReal(6, "torsionConstant");
        card.bendRadius = optionalOrNotGiven(reader, 7, "bendRadius");
        card.arcAngle = optionalOrNotGiven(reader, 8, "arcAngle");
        for (int index = 0; index < 8; ++index)
            card.stressPoints[static_cast<std::size_t>(index)] = reader.real(9 + index, STRESS_NAMES[index], 0.0);
        card.shearFactor1 = reader.real(17, "shearFactor1", 0.0);
        card.shearFactor2 = reader.real(18, "shearFactor2", 0.0);
        card.nonStructMass = reader.real(19, "nonStructMass", 0.0);
        card.centroidRadialOffset = reader.real(20, "centroidRadialOffset", 0.0);
        card.centerZ = reader.real(21, "centerZ", 0.0);
        card.neutralAxisOffset = optionalOrNotGiven(reader, 22, "neutralAxisOffset");
        if (reader.lastUsedField() > 22)
            reader.error(
                "card.bad-value",
                "a " + std::string(reader.cardName()) + " has no field after " + std::string("neutralAxisOffset"),
                "neutralAxisOffset");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CurvedBeamPropFormat::check(const CurvedBeamPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    // A deck can write 0.0 where the CBEND gives the value: only a negative one is refused.
    if (!std::isnan(card.bendRadius) && card.bendRadius < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "bendRadius"};
    if (!std::isnan(card.arcAngle) && card.arcAngle < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "arcAngle"};
    if (card.form == CurvedBeamPropForm::Section)
    {
        if (!(card.area > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "area"};
        if (!(card.bendingInertia1 > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "bendingInertia1"};
        if (!(card.bendingInertia2 > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "bendingInertia2"};
        if (!(card.torsionConstant > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "torsionConstant"};
        if (card.shearFactor1 < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "shearFactor1"};
        if (card.shearFactor2 < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "shearFactor2"};
        if (card.flexibilityOption != 0 || card.meanRadius != 0.0 || card.thickness != 0.0 ||
            !std::isnan(card.internalPressure))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("meanRadius") + ", " +
                                   CardProblem::fieldNamed("thickness") + " and " +
                                   CardProblem::fieldNamed("internalPressure") + " belong to the pipe format",
                               "flexibilityOption"};
        return std::nullopt;
    }
    if (card.flexibilityOption < 1 || card.flexibilityOption > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3",
                           "flexibilityOption"};
    if (!(card.meanRadius > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "meanRadius"};
    if (card.thickness < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "thickness"};
    if (card.thickness == 0.0 && card.flexibilityOption != 1)
        return CardProblem{"card.bad-value",
                           "a solid section (" + CardProblem::fieldNamed("thickness") + " = 0.0) needs " +
                               std::string(CardProblem::FIELD) + " = 1",
                           "flexibilityOption"};
    bool sectionValuesKept = card.area != 0.0 || card.bendingInertia1 != 0.0 || card.bendingInertia2 != 0.0 ||
                             card.torsionConstant != 0.0 || card.shearFactor1 != 0.0 || card.shearFactor2 != 0.0 ||
                             !std::isnan(card.neutralAxisOffset);
    for (const double value : card.stressPoints)
        sectionValuesKept = sectionValuesKept || value != 0.0;
    if (sectionValuesKept)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("bendingInertia1") + ", " +
                               CardProblem::fieldNamed("bendingInertia2") + ", " +
                               CardProblem::fieldNamed("torsionConstant") + ", the stress points, " +
                               CardProblem::fieldNamed("shearFactor1") + ", " +
                               CardProblem::fieldNamed("shearFactor2") + " and " +
                               CardProblem::fieldNamed("neutralAxisOffset") + " belong to the general format",
                           "area"};
    return std::nullopt;
}

void CurvedBeamPropFormat::write(const CurvedBeamPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    if (card.form == CurvedBeamPropForm::Pipe)
    {
        writer.integer(3, card.flexibilityOption);
        writer.real(4, card.meanRadius);
        writer.real(5, card.thickness);
        if (!std::isnan(card.internalPressure))
            writer.real(6, card.internalPressure);
        if (!std::isnan(card.bendRadius))
            writer.real(7, card.bendRadius);
        if (!std::isnan(card.arcAngle))
            writer.real(8, card.arcAngle);
        writer.realUnlessDefault(11, card.nonStructMass, 0.0);
        writer.realUnlessDefault(12, card.centroidRadialOffset, 0.0);
        writer.realUnlessDefault(13, card.centerZ, 0.0);
        return;
    }
    writer.real(3, card.area);
    writer.real(4, card.bendingInertia1);
    writer.real(5, card.bendingInertia2);
    writer.real(6, card.torsionConstant);
    if (!std::isnan(card.bendRadius))
        writer.real(7, card.bendRadius);
    if (!std::isnan(card.arcAngle))
        writer.real(8, card.arcAngle);
    for (int index = 0; index < 8; ++index)
        writer.realUnlessDefault(9 + index, card.stressPoints[static_cast<std::size_t>(index)], 0.0);
    writer.realUnlessDefault(17, card.shearFactor1, 0.0);
    writer.realUnlessDefault(18, card.shearFactor2, 0.0);
    writer.realUnlessDefault(19, card.nonStructMass, 0.0);
    writer.realUnlessDefault(20, card.centroidRadialOffset, 0.0);
    writer.realUnlessDefault(21, card.centerZ, 0.0);
    if (!std::isnan(card.neutralAxisOffset))
        writer.real(22, card.neutralAxisOffset);
}

std::uint32_t CurvedBeamPropTable::append(const CurvedBeamPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_form.push(static_cast<std::uint8_t>(card.form));
    m_section.push({card.area, card.bendingInertia1, card.bendingInertia2, card.torsionConstant});
    m_stressPoints.push(card.stressPoints);
    m_shearFactor.push({card.shearFactor1, card.shearFactor2});
    m_neutralAxisOffset.push(card.neutralAxisOffset);
    m_flexibilityOption.push(card.flexibilityOption);
    m_meanRadius.push(card.meanRadius);
    m_thickness.push(card.thickness);
    m_internalPressure.push(card.internalPressure);
    m_bendRadius.push(card.bendRadius);
    m_arcAngle.push(card.arcAngle);
    m_nonStructMass.push(card.nonStructMass);
    m_centroidRadialOffset.push(card.centroidRadialOffset);
    m_centerZ.push(card.centerZ);
    return row;
}

CurvedBeamPropCard CurvedBeamPropTable::card(std::uint32_t row) const
{
    CurvedBeamPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.form = static_cast<CurvedBeamPropForm>(m_form.get(row));
    const std::array<double, 4> section = m_section.get(row);
    card.area = section[0];
    card.bendingInertia1 = section[1];
    card.bendingInertia2 = section[2];
    card.torsionConstant = section[3];
    card.stressPoints = m_stressPoints.get(row);
    const std::array<double, 2> shear = m_shearFactor.get(row);
    card.shearFactor1 = shear[0];
    card.shearFactor2 = shear[1];
    card.neutralAxisOffset = m_neutralAxisOffset.get(row);
    card.flexibilityOption = m_flexibilityOption.get(row);
    card.meanRadius = m_meanRadius.get(row);
    card.thickness = m_thickness.get(row);
    card.internalPressure = m_internalPressure.get(row);
    card.bendRadius = m_bendRadius.get(row);
    card.arcAngle = m_arcAngle.get(row);
    card.nonStructMass = m_nonStructMass.get(row);
    card.centroidRadialOffset = m_centroidRadialOffset.get(row);
    card.centerZ = m_centerZ.get(row);
    return card;
}

void CurvedBeamPropTable::store(std::uint32_t row, const CurvedBeamPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_form.set(row, static_cast<std::uint8_t>(card.form));
    m_section.set(row, {card.area, card.bendingInertia1, card.bendingInertia2, card.torsionConstant});
    m_stressPoints.set(row, card.stressPoints);
    m_shearFactor.set(row, {card.shearFactor1, card.shearFactor2});
    m_neutralAxisOffset.set(row, card.neutralAxisOffset);
    m_flexibilityOption.set(row, card.flexibilityOption);
    m_meanRadius.set(row, card.meanRadius);
    m_thickness.set(row, card.thickness);
    m_internalPressure.set(row, card.internalPressure);
    m_bendRadius.set(row, card.bendRadius);
    m_arcAngle.set(row, card.arcAngle);
    m_nonStructMass.set(row, card.nonStructMass);
    m_centroidRadialOffset.set(row, card.centroidRadialOffset);
    m_centerZ.set(row, card.centerZ);
}

void CurvedBeamPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_form.keepRows(rows);
    m_section.keepRows(rows);
    m_stressPoints.keepRows(rows);
    m_shearFactor.keepRows(rows);
    m_neutralAxisOffset.keepRows(rows);
    m_flexibilityOption.keepRows(rows);
    m_meanRadius.keepRows(rows);
    m_thickness.keepRows(rows);
    m_internalPressure.keepRows(rows);
    m_bendRadius.keepRows(rows);
    m_arcAngle.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_centroidRadialOffset.keepRows(rows);
    m_centerZ.keepRows(rows);
}

void CurvedBeamPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
