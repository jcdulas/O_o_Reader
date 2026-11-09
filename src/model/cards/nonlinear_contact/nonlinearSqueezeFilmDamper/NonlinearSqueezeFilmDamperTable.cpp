#include "oor/model/cards/nonlinear_contact/nonlinearSqueezeFilmDamper/NonlinearSqueezeFilmDamperTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The fields of the card: 4 lines of 8.
constexpr int VISCOSITY_FIELD = 9;
constexpr int FIRST_OFFSET_FIELD = 17;
constexpr int FIRST_SERVICE_PARAMETER_FIELD = 25;

bool isAngle(double theta)
{
    return theta >= 0.0 && theta <= 360.0;
}

// A field that the card needs unless the damper comes from an external service: it is 0.0 in the card when blank there.
// Written blank when it is 0.0 in the external form, always otherwise (a pressure of 0.0 is a value of the card).
void writeCharacteristic(FieldWriter& writer, int field, double value, bool external)
{
    if (external)
        writer.realUnlessDefault(field, value, 0.0);
    else
        writer.real(field, value);
}

} // namespace

std::optional<NonlinearSqueezeFilmDamperCard> NonlinearSqueezeFilmDamperFormat::read(FieldReader& reader)
{
    NonlinearSqueezeFilmDamperCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.plane = reader.isBlank(4) ? "PlaneXy" : std::string(reader.text(4));
    card.serviceGroup = reader.word(19);
    card.serviceEvaluation = reader.word(20);
    const bool external = !card.serviceGroup.empty();
    const auto characteristic = [&](int field, const char* name)
    { return external ? reader.real(field, name, 0.0) : reader.requiredReal(field, name); };
    card.bearingDiameter = characteristic(5, "bearingDiameter");
    card.bearingLength = characteristic(6, "bearingLength");
    card.bearingClearance = characteristic(7, "bearingClearance");
    card.bearingKind = reader.isBlank(8) ? "Long" : std::string(reader.text(8));
    card.viscosity = characteristic(VISCOSITY_FIELD, "viscosity");
    card.vapourPressure = characteristic(VISCOSITY_FIELD + 1, "vapourPressure");
    card.portCount = external ? reader.integer(VISCOSITY_FIELD + 2, "portCount", 0)
                              : reader.requiredInteger(VISCOSITY_FIELD + 2, "portCount");
    // The pressure and the angle of a port are required with the ports there are.
    const bool port1 = !external && card.portCount >= 1;
    const bool port2 = !external && card.portCount == 2;
    card.pressure1 = port1 ? reader.requiredReal(VISCOSITY_FIELD + 3, "pressure1")
                           : reader.real(VISCOSITY_FIELD + 3, "pressure1", 0.0);
    card.portAngle1 = port1 ? reader.requiredReal(VISCOSITY_FIELD + 4, "portAngle1")
                            : reader.real(VISCOSITY_FIELD + 4, "portAngle1", 0.0);
    card.pressure2 = port2 ? reader.requiredReal(VISCOSITY_FIELD + 5, "pressure2")
                           : reader.real(VISCOSITY_FIELD + 5, "pressure2", 0.0);
    card.portAngle2 = port2 ? reader.requiredReal(VISCOSITY_FIELD + 6, "portAngle2")
                            : reader.real(VISCOSITY_FIELD + 6, "portAngle2", 0.0);
    card.pointCount = reader.integer(VISCOSITY_FIELD + 7, "pointCount", 101);
    card.offset1 = reader.real(FIRST_OFFSET_FIELD, "offset1", 0.0);
    card.offset2 = reader.real(FIRST_OFFSET_FIELD + 1, "offset2", 0.0);
    for (std::size_t index = 0; index < card.serviceParameters.size(); ++index)
        card.serviceParameters[index] =
            reader.real(FIRST_SERVICE_PARAMETER_FIELD + static_cast<int>(index), "serviceParameters", 0.0);
    requireUnusedBlank(reader,
                       {FIRST_OFFSET_FIELD + 4, FIRST_OFFSET_FIELD + 5, FIRST_OFFSET_FIELD + 6, FIRST_OFFSET_FIELD + 7},
                       FIRST_SERVICE_PARAMETER_FIELD + 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearSqueezeFilmDamperFormat::check(const NonlinearSqueezeFilmDamperCard& card)
{
    if (card.setId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "setId"};
    if (card.nodeA <= 0 || card.nodeB <= 0)
        return CardProblem{"card.bad-id",
                           CardProblem::fieldNamed("nodeA") + " and " + CardProblem::fieldNamed("nodeB") +
                               " must be positive grid point IDs",
                           card.nodeA <= 0 ? "nodeA" : "nodeB"};
    // The format reference describes the plane as XY, XZ or ZX in one place and as XY, YZ or ZX in another.
    if (auto problem = checkKeyword(card.plane, "plane", {"PlaneXy", "PlaneYz", "PlaneZx", "PlaneXz"}))
        return problem;
    if (auto problem = checkKeyword(card.bearingKind, "bearingKind", {"Long", "Short"}))
        return problem;
    for (const double value :
         {card.bearingDiameter, card.bearingLength, card.bearingClearance, card.viscosity, card.vapourPressure,
          card.pressure1, card.portAngle1, card.pressure2, card.portAngle2, card.offset1, card.offset2})
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "a field of the card must be a finite number", "values"};
    }
    // The characteristics are ignored when an external service gives them.
    const bool external = !card.serviceGroup.empty();
    if (!external)
    {
        if (!(card.bearingDiameter > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "bearingDiameter"};
        if (!(card.bearingLength > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "bearingLength"};
        if (!(card.bearingClearance > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "bearingClearance"};
        if (!(card.viscosity > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "viscosity"};
        if (card.portCount != 1 && card.portCount != 2)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "portCount"};
        if (card.pressure1 < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "pressure1"};
        if (!isAngle(card.portAngle1))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 0 and 360 degrees",
                               "portAngle1"};
        if (card.portCount == 2)
        {
            if (card.pressure2 < 0.0)
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                                   "pressure2"};
            if (!isAngle(card.portAngle2))
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " must be between 0 and 360 degrees",
                                   "portAngle2"};
        }
    }
    if (card.pointCount <= 0 || card.pointCount >= 201 || card.pointCount % 2 == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an odd integer below 201",
                           "pointCount"};
    for (const double value : card.serviceParameters)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "serviceParameters"};
    }
    return std::nullopt;
}

void NonlinearSqueezeFilmDamperFormat::write(const NonlinearSqueezeFilmDamperCard& card, FieldWriter& writer)
{
    const bool external = !card.serviceGroup.empty();
    writer.integer(1, card.setId);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writer.textUnlessDefault(4, card.plane, "PlaneXy");
    writeCharacteristic(writer, 5, card.bearingDiameter, external);
    writeCharacteristic(writer, 6, card.bearingLength, external);
    writeCharacteristic(writer, 7, card.bearingClearance, external);
    writer.textUnlessDefault(8, card.bearingKind, "Long");
    writeCharacteristic(writer, VISCOSITY_FIELD, card.viscosity, external);
    writeCharacteristic(writer, VISCOSITY_FIELD + 1, card.vapourPressure, external);
    if (!external || card.portCount != 0)
        writer.integer(VISCOSITY_FIELD + 2, card.portCount);
    writeCharacteristic(writer, VISCOSITY_FIELD + 3, card.pressure1, external || card.portCount < 1);
    writeCharacteristic(writer, VISCOSITY_FIELD + 4, card.portAngle1, external || card.portCount < 1);
    writeCharacteristic(writer, VISCOSITY_FIELD + 5, card.pressure2, external || card.portCount < 2);
    writeCharacteristic(writer, VISCOSITY_FIELD + 6, card.portAngle2, external || card.portCount < 2);
    writer.integerUnlessDefault(VISCOSITY_FIELD + 7, card.pointCount, 101);
    writer.realUnlessDefault(FIRST_OFFSET_FIELD, card.offset1, 0.0);
    writer.realUnlessDefault(FIRST_OFFSET_FIELD + 1, card.offset2, 0.0);
    if (external)
        writer.text(FIRST_OFFSET_FIELD + 2, card.serviceGroup);
    if (!card.serviceEvaluation.empty())
        writer.text(FIRST_OFFSET_FIELD + 3, card.serviceEvaluation);
    for (std::size_t index = 0; index < card.serviceParameters.size(); ++index)
        writer.realUnlessDefault(FIRST_SERVICE_PARAMETER_FIELD + static_cast<int>(index), card.serviceParameters[index],
                                 0.0);
}

std::uint32_t NonlinearSqueezeFilmDamperTable::append(const NonlinearSqueezeFilmDamperCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void NonlinearSqueezeFilmDamperTable::store(std::uint32_t row, const NonlinearSqueezeFilmDamperCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void NonlinearSqueezeFilmDamperTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearSqueezeFilmDamperTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void NonlinearSqueezeFilmDamperTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    NonlinearSqueezeFilmDamperCard& card = m_cards.edit(row);
    card.nodeA = visitor.visit("nodeA", ReferenceTarget::Node, card.nodeA);
    card.nodeB = visitor.visit("nodeB", ReferenceTarget::Node, card.nodeB);
}

} // namespace oor::model
