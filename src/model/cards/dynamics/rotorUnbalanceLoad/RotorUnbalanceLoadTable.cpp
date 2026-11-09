#include "oor/model/cards/dynamics/rotorUnbalanceLoad/RotorUnbalanceLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/dynamics/DynamicLoadFields.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

// The field of each EPOINT: UFT1..3 and UFR1..3 on the third line, MCT1..3 and MCR1..3 on the fourth, SCR1..3 on the
// fifth, each line starting at field 17, 25 and 33.
constexpr int EXTRA_POINT_FIELDS[RotorUnbalanceLoadCard::EXTRA_POINT_COUNT] = {17, 18, 19, 20, 21, 22, 25, 26,
                                                                               27, 28, 29, 30, 33, 34, 35};
constexpr const char* EXTRA_POINT_NAMES[RotorUnbalanceLoadCard::EXTRA_POINT_COUNT] = {
    "forcePointT1",          "forcePointT2",          "forcePointT3",          "forcePointR1",
    "forcePointR2",          "forcePointR3",          "massCorrectionPointT1", "massCorrectionPointT2",
    "massCorrectionPointT3", "massCorrectionPointR1", "massCorrectionPointR2", "massCorrectionPointR3",
    "speedCorrectionPoint1", "speedCorrectionPoint2", "speedCorrectionPoint3"};

const char* correctionWord(UnbalanceCorrection correction)
{
    switch (correction)
    {
    case UnbalanceCorrection::None:
        return "None";
    case UnbalanceCorrection::Mass:
        return "Mass";
    case UnbalanceCorrection::Speed:
        return "Speed";
    case UnbalanceCorrection::Both:
        return "Both";
    }
    return "None";
}

// A field that is a real or an ID where the default is not zero: blank is `fallback`.
void readIdOrRealWithDefault(FieldReader& reader, int field, const char* name, base::EntityId& id, double& value,
                             double fallback)
{
    if (reader.isBlank(field))
    {
        id = 0;
        value = fallback;
        return;
    }
    readIdOrReal(reader, field, name, id, value);
}

void writeIdOrRealWithDefault(FieldWriter& writer, int field, base::EntityId id, double value, double fallback)
{
    if (id != 0)
        writer.integer(field, id);
    else
        writer.realUnlessDefault(field, value, fallback);
}

} // namespace

std::optional<RotorUnbalanceLoadCard> RotorUnbalanceLoadFormat::read(FieldReader& reader)
{
    RotorUnbalanceLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    if (reader.isBlank(2))
        reader.error("card.missing-field", std::string("mass") + " is required", "mass");
    readIdOrReal(reader, 2, "mass", card.massTableId, card.mass);
    card.nodeId = reader.requiredInteger(3, "nodeId");
    card.direction[0] = reader.requiredReal(4, "directionX");
    card.direction[1] = reader.requiredReal(5, "directionY");
    card.direction[2] = reader.requiredReal(6, "directionZ");
    for (const int field : {7, 8})
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 7 and 8 of the card are not used", "directionZ");
    }
    readIdOrRealWithDefault(reader, 9, "radialOffset", card.radialOffsetTableId, card.radialOffset, 1.0);
    card.angularPosition = reader.real(10, "angularPosition", 0.0);
    readIdOrRealWithDefault(reader, 11, "axialOffset", card.axialOffsetTableId, card.axialOffset, 0.0);
    card.startTimeOrFrequency = reader.real(12, "time/startTimeOrFrequency", 0.0);
    card.stopTimeOrFrequency = reader.real(13, "time/stopTimeOrFrequency", 999999.0);
    const std::string_view correction = reader.text(14);
    if (correction.empty() || (correction == "None"))
        card.correctionFlag = UnbalanceCorrection::None;
    else if ((correction == "Mass"))
        card.correctionFlag = UnbalanceCorrection::Mass;
    else if ((correction == "Speed"))
        card.correctionFlag = UnbalanceCorrection::Speed;
    else if ((correction == "Both"))
        card.correctionFlag = UnbalanceCorrection::Both;
    else
        reader.error("card.bad-value",
                     std::string("correctionFlag") + " must be NONE, MASS, SPEED or BOTH, found \"" +
                         std::string(correction) + "\"",
                     "correctionFlag");
    for (const int field : {15, 16, 23, 24, 31, 32})
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "field " + std::to_string(field) + " of the card is not used",
                         "correctionFlag");
    }
    for (int index = 0; index < RotorUnbalanceLoadCard::EXTRA_POINT_COUNT; ++index)
        card.extraPoints[static_cast<std::size_t>(index)] =
            reader.integer(EXTRA_POINT_FIELDS[index], EXTRA_POINT_NAMES[index], 0);
    if (reader.lastUsedField() > 35)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("speedCorrectionPoint3"),
                     "speedCorrectionPoint3");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorUnbalanceLoadFormat::check(const RotorUnbalanceLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (const auto problem = checkIdOrReal(card.massTableId, card.mass, "mass"))
        return problem;
    if (card.massTableId == 0 && !std::isfinite(card.mass))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "mass"};
    if (card.nodeId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "nodeId"};
    for (const double component : card.direction)
    {
        if (!std::isfinite(component))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("directionY") +
                                   " and " + CardProblem::fieldNamed("directionZ") + " must be finite numbers",
                               "directionX"};
    }
    if (const auto problem = checkIdOrReal(card.radialOffsetTableId, card.radialOffset, "radialOffset"))
        return problem;
    if (!std::isfinite(card.angularPosition))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "angularPosition"};
    if (const auto problem = checkIdOrReal(card.axialOffsetTableId, card.axialOffset, "axialOffset"))
        return problem;
    if (!(card.startTimeOrFrequency >= 0.0) || !std::isfinite(card.startTimeOrFrequency))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "time/startTimeOrFrequency"};
    if (!(card.stopTimeOrFrequency > 0.0) || !std::isfinite(card.stopTimeOrFrequency))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "time/stopTimeOrFrequency"};
    for (int index = 0; index < RotorUnbalanceLoadCard::EXTRA_POINT_COUNT; ++index)
    {
        if (card.extraPoints[static_cast<std::size_t>(index)] < 0)
            return CardProblem{
                "card.bad-id",
                "the extra point ID " + std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                EXTRA_POINT_NAMES[index]};
    }
    return std::nullopt;
}

void RotorUnbalanceLoadFormat::write(const RotorUnbalanceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.massTableId != 0) // MASS is required: a real zero is written
        writer.integer(2, card.massTableId);
    else
        writer.real(2, card.mass);
    writer.integer(3, card.nodeId);
    writer.real(4, card.direction[0]);
    writer.real(5, card.direction[1]);
    writer.real(6, card.direction[2]);
    writeIdOrRealWithDefault(writer, 9, card.radialOffsetTableId, card.radialOffset, 1.0);
    writer.realUnlessDefault(10, card.angularPosition, 0.0);
    writeIdOrReal(writer, 11, card.axialOffsetTableId, card.axialOffset);
    writer.realUnlessDefault(12, card.startTimeOrFrequency, 0.0);
    writer.realUnlessDefault(13, card.stopTimeOrFrequency, 999999.0);
    writer.textUnlessDefault(14, correctionWord(card.correctionFlag), "None");
    for (int index = 0; index < RotorUnbalanceLoadCard::EXTRA_POINT_COUNT; ++index)
    {
        if (card.extraPoints[static_cast<std::size_t>(index)] != 0)
            writer.integer(EXTRA_POINT_FIELDS[index], card.extraPoints[static_cast<std::size_t>(index)]);
    }
}

void RotorUnbalanceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeId.set(row, visitor.visit("nodeId", ReferenceTarget::Node, m_nodeId.get(row)));
    m_massTableId.set(row, visitIfSet(visitor, "mass", ReferenceTarget::DynamicTable, m_massTableId.get(row)));
    m_radialOffsetTableId.set(
        row, visitIfSet(visitor, "radialOffset", ReferenceTarget::DynamicTable, m_radialOffsetTableId.get(row)));
    m_axialOffsetTableId.set(
        row, visitIfSet(visitor, "axialOffset", ReferenceTarget::DynamicTable, m_axialOffsetTableId.get(row)));
    std::array<base::EntityId, RotorUnbalanceLoadCard::EXTRA_POINT_COUNT> extraPoints = m_extraPoints.get(row);
    for (int index = 0; index < RotorUnbalanceLoadCard::EXTRA_POINT_COUNT; ++index)
    {
        base::EntityId& point = extraPoints[static_cast<std::size_t>(index)];
        point = visitIfSet(visitor, EXTRA_POINT_NAMES[index], ReferenceTarget::Node, point);
    }
    m_extraPoints.set(row, extraPoints);
}

std::uint32_t RotorUnbalanceLoadTable::append(const RotorUnbalanceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_massTableId.push(card.massTableId);
    m_mass.push(card.mass);
    m_nodeId.push(card.nodeId);
    m_direction.push(card.direction);
    m_radialOffsetTableId.push(card.radialOffsetTableId);
    m_radialOffset.push(card.radialOffset);
    m_angularPosition.push(card.angularPosition);
    m_axialOffsetTableId.push(card.axialOffsetTableId);
    m_axialOffset.push(card.axialOffset);
    m_startTimeOrFrequency.push(card.startTimeOrFrequency);
    m_stopTimeOrFrequency.push(card.stopTimeOrFrequency);
    m_correctionFlag.push(static_cast<std::uint8_t>(card.correctionFlag));
    m_extraPoints.push(card.extraPoints);
    return row;
}

RotorUnbalanceLoadCard RotorUnbalanceLoadTable::card(std::uint32_t row) const
{
    RotorUnbalanceLoadCard card;
    card.id = m_id.get(row);
    card.massTableId = m_massTableId.get(row);
    card.mass = m_mass.get(row);
    card.nodeId = m_nodeId.get(row);
    card.direction = m_direction.get(row);
    card.radialOffsetTableId = m_radialOffsetTableId.get(row);
    card.radialOffset = m_radialOffset.get(row);
    card.angularPosition = m_angularPosition.get(row);
    card.axialOffsetTableId = m_axialOffsetTableId.get(row);
    card.axialOffset = m_axialOffset.get(row);
    card.startTimeOrFrequency = m_startTimeOrFrequency.get(row);
    card.stopTimeOrFrequency = m_stopTimeOrFrequency.get(row);
    card.correctionFlag = static_cast<UnbalanceCorrection>(m_correctionFlag.get(row));
    card.extraPoints = m_extraPoints.get(row);
    return card;
}

void RotorUnbalanceLoadTable::store(std::uint32_t row, const RotorUnbalanceLoadCard& card)
{
    m_id.set(row, card.id);
    m_massTableId.set(row, card.massTableId);
    m_mass.set(row, card.mass);
    m_nodeId.set(row, card.nodeId);
    m_direction.set(row, card.direction);
    m_radialOffsetTableId.set(row, card.radialOffsetTableId);
    m_radialOffset.set(row, card.radialOffset);
    m_angularPosition.set(row, card.angularPosition);
    m_axialOffsetTableId.set(row, card.axialOffsetTableId);
    m_axialOffset.set(row, card.axialOffset);
    m_startTimeOrFrequency.set(row, card.startTimeOrFrequency);
    m_stopTimeOrFrequency.set(row, card.stopTimeOrFrequency);
    m_correctionFlag.set(row, static_cast<std::uint8_t>(card.correctionFlag));
    m_extraPoints.set(row, card.extraPoints);
}

void RotorUnbalanceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_massTableId.keepRows(rows);
    m_mass.keepRows(rows);
    m_nodeId.keepRows(rows);
    m_direction.keepRows(rows);
    m_radialOffsetTableId.keepRows(rows);
    m_radialOffset.keepRows(rows);
    m_angularPosition.keepRows(rows);
    m_axialOffsetTableId.keepRows(rows);
    m_axialOffset.keepRows(rows);
    m_startTimeOrFrequency.keepRows(rows);
    m_stopTimeOrFrequency.keepRows(rows);
    m_correctionFlag.keepRows(rows);
    m_extraPoints.keepRows(rows);
}

} // namespace oor::model
