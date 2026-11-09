#include "oor/model/cards/properties/bushProp2D/BushProp2DTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <vector>

namespace oor::model
{

namespace
{

// The fields of the card: the nominal values on the first line, the option from field 9 on the second, and the ports of
// a squeeze film damper on the third (field 8 and 16 are blank cells of the grid).
constexpr int LAST_NOMINAL_FIELD = 7;
constexpr int OPTION_FIELD = 9;
constexpr int LAST_OPTION_FIELD = 15;
constexpr int FIRST_PORT_FIELD = 17;
constexpr int LAST_PORT_FIELD = 23;

void appendFields(std::vector<int>& fields, int first, int last)
{
    for (int field = first; field <= last; ++field)
        fields.push_back(field);
}

// The fields a form of the card uses, for the check that the others are blank.
std::vector<int> fieldsOfForm(bool hasOption, bool hasPorts)
{
    std::vector<int> fields;
    appendFields(fields, 1, LAST_NOMINAL_FIELD);
    if (hasOption)
        appendFields(fields, OPTION_FIELD, LAST_OPTION_FIELD);
    if (hasPorts)
        appendFields(fields, FIRST_PORT_FIELD, LAST_PORT_FIELD);
    return fields;
}

BushProp2DSqueeze readSqueeze(FieldReader& reader)
{
    BushProp2DSqueeze squeeze;
    squeeze.bearingDiameter = reader.requiredReal(10, "bearingDiameter");
    squeeze.bearingLength = reader.requiredReal(11, "bearingLength");
    squeeze.bearingClearance = reader.requiredReal(12, "bearingClearance");
    squeeze.bearingModel =
        readKeyword(reader, 13, "bearingModel", BUSH_PROP_2D_SOLUTION_WORDS, BushProp2DSolution::Long);
    squeeze.viscosity = reader.requiredReal(14, "viscosity");
    squeeze.vaporPressure = reader.requiredReal(15, "vaporPressure");
    // NPORT has no default, so a card with no third line is incomplete.
    squeeze.portCount = reader.requiredInteger(17, "portCount");
    squeeze.pressure1 = reader.requiredReal(18, "pressure1");
    squeeze.portAngle1 = reader.requiredReal(19, "portAngle1");
    squeeze.pressure2 = reader.optionalReal(20, "pressure2");
    squeeze.portAngle2 = reader.optionalReal(21, "portAngle2");
    squeeze.offset1 = reader.real(22, "offset1", 0.0);
    squeeze.offset2 = reader.real(23, "offset2", 0.0);
    return squeeze;
}

BushProp2DCross readCross(FieldReader& reader)
{
    // The format reference gives the terms neither a default nor a "required" mark: blank is 0.0.
    BushProp2DCross cross;
    cross.stiffness12 = reader.real(10, "stiffness12", 0.0);
    cross.stiffness21 = reader.real(11, "stiffness21", 0.0);
    cross.damping12 = reader.real(12, "damping12", 0.0);
    cross.damping21 = reader.real(13, "damping21", 0.0);
    cross.mass12 = reader.real(14, "mass12", 0.0);
    cross.mass21 = reader.real(15, "mass21", 0.0);
    return cross;
}

// An angular position of a lubrication port: 0.0 <= angle < 360.0 degrees.
std::optional<CardProblem> checkAngle(double angle, const char* field)
{
    if (!(angle >= 0.0 && angle < 360.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be at least 0.0 and less than 360.0", field};
    return std::nullopt;
}

std::optional<CardProblem> checkSqueeze(const BushProp2DSqueeze& squeeze)
{
    if (const auto problem = checkReal(squeeze.bearingDiameter, "bearingDiameter", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(squeeze.bearingLength, "bearingLength", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(squeeze.bearingClearance, "bearingClearance", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(squeeze.viscosity, "viscosity", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(squeeze.vaporPressure, "vaporPressure", RealRule::Finite))
        return problem;
    if (squeeze.portCount != 1 && squeeze.portCount != 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "portCount"};
    if (const auto problem = checkReal(squeeze.pressure1, "pressure1", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkAngle(squeeze.portAngle1, "portAngle1"))
        return problem;
    // The second port is required for two ports. A one-port card that gives it is kept: the format does not forbid it.
    if (squeeze.portCount == 2 && !squeeze.pressure2)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required when " + CardProblem::fieldNamed("portCount") + " is 2",
            "pressure2"};
    if (squeeze.portCount == 2 && !squeeze.portAngle2)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required when " + CardProblem::fieldNamed("portCount") + " is 2",
            "portAngle2"};
    if (const auto problem = checkOptionalReal(squeeze.pressure2, "pressure2", RealRule::NonNegative))
        return problem;
    if (squeeze.portAngle2)
    {
        if (const auto problem = checkAngle(*squeeze.portAngle2, "portAngle2"))
            return problem;
    }
    if (const auto problem = checkReal(squeeze.offset1, "offset1", RealRule::Finite))
        return problem;
    return checkReal(squeeze.offset2, "offset2", RealRule::Finite);
}

std::optional<CardProblem> checkCross(const BushProp2DCross& cross)
{
    if (const auto problem = checkReal(cross.stiffness12, "stiffness12", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(cross.stiffness21, "stiffness21", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(cross.damping12, "damping12", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(cross.damping21, "damping21", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(cross.mass12, "mass12", RealRule::Finite))
        return problem;
    return checkReal(cross.mass21, "mass21", RealRule::Finite);
}

void writeSqueeze(const BushProp2DSqueeze& squeeze, FieldWriter& writer)
{
    writer.text(OPTION_FIELD, BUSH_PROP_2D_SQUEEZE_WORD);
    writer.real(10, squeeze.bearingDiameter);
    writer.real(11, squeeze.bearingLength);
    writer.real(12, squeeze.bearingClearance);
    writeKeywordUnlessDefault(writer, 13, BUSH_PROP_2D_SOLUTION_WORDS, squeeze.bearingModel, BushProp2DSolution::Long);
    writer.real(14, squeeze.viscosity);
    writer.real(15, squeeze.vaporPressure);
    writer.integer(17, squeeze.portCount);
    writer.real(18, squeeze.pressure1);
    writer.real(19, squeeze.portAngle1);
    if (squeeze.pressure2)
        writer.real(20, *squeeze.pressure2);
    if (squeeze.portAngle2)
        writer.real(21, *squeeze.portAngle2);
    writer.realUnlessDefault(22, squeeze.offset1, 0.0);
    writer.realUnlessDefault(23, squeeze.offset2, 0.0);
}

void writeCross(const BushProp2DCross& cross, FieldWriter& writer)
{
    writer.text(OPTION_FIELD, BUSH_PROP_2D_CROSS_WORD);
    writer.realUnlessDefault(10, cross.stiffness12, 0.0);
    writer.realUnlessDefault(11, cross.stiffness21, 0.0);
    writer.realUnlessDefault(12, cross.damping12, 0.0);
    writer.realUnlessDefault(13, cross.damping21, 0.0);
    writer.realUnlessDefault(14, cross.mass12, 0.0);
    writer.realUnlessDefault(15, cross.mass21, 0.0);
}

} // namespace

std::optional<BushProp2DCard> BushProp2DFormat::read(FieldReader& reader)
{
    BushProp2DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stiffness11 = reader.requiredReal(2, "stiffness11");
    card.stiffness22 = reader.requiredReal(3, "stiffness22");
    card.damping11 = reader.real(4, "damping11", 0.0);
    card.damping22 = reader.real(5, "damping22", 0.0);
    card.mass11 = reader.real(6, "mass11", 0.0);
    card.mass22 = reader.real(7, "mass22", 0.0);
    // The word in field 9 says which option follows: SQUEEZE (three lines), CROSS (two lines) or none (one line).
    const std::string_view option = reader.text(OPTION_FIELD);
    if (option.empty())
    {
        requireOnlyFields(reader, fieldsOfForm(false, false));
    }
    else if ((option == BUSH_PROP_2D_SQUEEZE_WORD))
    {
        card.squeeze = readSqueeze(reader);
        requireOnlyFields(reader, fieldsOfForm(true, true));
    }
    else if ((option == BUSH_PROP_2D_CROSS_WORD))
    {
        card.crossCoupling = readCross(reader);
        requireOnlyFields(reader, fieldsOfForm(true, false));
    }
    else
    {
        reader.error("card.bad-value",
                     "field 9 of a " + std::string(reader.cardName()) + " must be SQUEEZE or CROSS, not \"" +
                         std::string(option) + "\"",
                     "");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BushProp2DFormat::check(const BushProp2DCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.stiffness11, "stiffness11", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.stiffness22, "stiffness22", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.damping11, "damping11", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.damping22, "damping22", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.mass11, "mass11", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.mass22, "mass22", RealRule::Finite))
        return problem;
    // either SQUEEZE or CROSS, not both.
    if (card.squeeze && card.crossCoupling)
        return CardProblem{"card.bad-value", "the card has the SQUEEZE option or the CROSS option, not both",
                           "crossCoupling"};
    if (card.squeeze)
        return checkSqueeze(*card.squeeze);
    if (card.crossCoupling)
        return checkCross(*card.crossCoupling);
    return std::nullopt;
}

void BushProp2DFormat::write(const BushProp2DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.stiffness11);
    writer.real(3, card.stiffness22);
    writer.realUnlessDefault(4, card.damping11, 0.0);
    writer.realUnlessDefault(5, card.damping22, 0.0);
    writer.realUnlessDefault(6, card.mass11, 0.0);
    writer.realUnlessDefault(7, card.mass22, 0.0);
    if (card.squeeze)
        writeSqueeze(*card.squeeze, writer);
    else if (card.crossCoupling)
        writeCross(*card.crossCoupling, writer);
}

std::uint32_t BushProp2DTable::append(const BushProp2DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void BushProp2DTable::store(std::uint32_t row, const BushProp2DCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void BushProp2DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void BushProp2DTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
