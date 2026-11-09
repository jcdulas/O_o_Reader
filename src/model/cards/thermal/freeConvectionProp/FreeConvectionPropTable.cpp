#include "oor/model/cards/thermal/freeConvectionProp/FreeConvectionPropTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FORMULA_KIND_BY_AMBIENT_POINT = 3;

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> given(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

bool isForm(int form) noexcept
{
    return form == 0 || form == 1 || form == 10 || form == 11 || form == 20 || form == 21;
}

} // namespace

FreeConvectionPropTable::FreeConvectionPropTable()
    : m_characteristicLength(notGiven()), m_direction1(notGiven()), m_direction2(notGiven()), m_direction3(notGiven())
{
}

std::optional<FreeConvectionPropCard> FreeConvectionPropFormat::read(FieldReader& reader)
{
    FreeConvectionPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.integer(2, "matId", 0);
    card.matrixForm = reader.integer(3, "matrixForm", 0);
    card.convectionExponent = reader.real(4, "convectionExponent", 0.0);
    card.formulaKind = reader.integer(5, "formulaKind", 0);
    if (card.formulaKind == FORMULA_KIND_BY_AMBIENT_POINT)
    {
        // The alternate layout: the coefficient of each ambient point from field 6; a blank one is H1 (format
        // reference).
        const double convectionCoefficient1 = reader.requiredReal(6, "convectionCoefficient1");
        card.convectionCoefficient[0] = convectionCoefficient1;
        for (int index = 1; index < 8; ++index)
        {
            const std::string name = "H" + std::to_string(index + 1);
            card.convectionCoefficient[static_cast<std::size_t>(index)] =
                reader.real(6 + index, name.c_str(), convectionCoefficient1);
        }
        requireUnusedBlank(reader, {}, 13);
    }
    else
    {
        card.tableId = reader.integer(6, "tableId", 0);
        card.characteristicLength = reader.optionalReal(9, "characteristicLength");
        card.inletNode = reader.integer(10, "inletNode", 0);
        card.directionFrameId = reader.integer(11, "directionFrameId", 0);
        card.direction1 = reader.optionalReal(12, "direction1");
        card.direction2 = reader.optionalReal(13, "direction2");
        card.direction3 = reader.optionalReal(14, "direction3");
        requireUnusedBlank(reader, {7, 8}, 14);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreeConvectionPropFormat::check(const FreeConvectionPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the convection property ID must be a positive integer", "id"};
    if (card.matId < 0)
        return CardProblem{"card.bad-id", "the material ID must be positive (or blank)", "matId"};
    if (!isForm(card.matrixForm))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 10, 11, 20 or 21",
                           "matrixForm"};
    if (!(card.convectionExponent >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative",
                           "convectionExponent"};
    if (card.formulaKind < 0 || card.formulaKind > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 2 or 3", "formulaKind"};
    if (card.formulaKind == FORMULA_KIND_BY_AMBIENT_POINT)
    {
        // The alternate layout has no table, length, inlet point or flow direction.
        if (card.tableId != 0 || card.characteristicLength || card.inletNode != 0 || card.directionFrameId != 0 ||
            card.direction1 || card.direction2 || card.direction3)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " 3 has the coefficients " +
                    CardProblem::fieldNamed("convectionCoefficient1") + " to " +
                    CardProblem::fieldNamed("convectionCoefficient8") + ", not " + CardProblem::fieldNamed("tableId") +
                    ", " + CardProblem::fieldNamed("characteristicLength") + ", " +
                    CardProblem::fieldNamed("inletNode") + ", " + CardProblem::fieldNamed("directionFrameId") + " or " +
                    CardProblem::fieldNamed("direction1") + " to " + CardProblem::fieldNamed("direction3"),
                "formulaKind"};
        return std::nullopt;
    }
    for (const double value : card.convectionCoefficient)
    {
        if (value != 0.0)
            return CardProblem{"card.bad-value",
                               std::string("the coefficients ") + CardProblem::fieldNamed("convectionCoefficient1") +
                                   " to " + CardProblem::fieldNamed("convectionCoefficient8") + " go with " +
                                   CardProblem::fieldNamed("formulaKind") + " 3",
                               "convectionCoefficient"};
    }
    // MID gives the coefficient (FTYPE 0) or the conductivity (FTYPE 2); the tabular function
    // of FTYPE 1 and 2 is the TABLEHT of TID.
    if ((card.formulaKind == 0 || card.formulaKind == 2) && card.matId == 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required for " + CardProblem::fieldNamed("formulaKind") + " 0 and 2",
            "matId"};
    if ((card.formulaKind == 1 || card.formulaKind == 2) && card.tableId == 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " is required for " + CardProblem::fieldNamed("formulaKind") + " 1 and 2",
            "tableId"};
    if (card.tableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be positive (or blank)", "tableId"};
    if (card.characteristicLength && !(*card.characteristicLength > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "characteristicLength"};
    if (card.inletNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be positive (or blank)",
                           "inletNode"};
    if (card.directionFrameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "directionFrameId"};
    return std::nullopt;
}

void FreeConvectionPropFormat::write(const FreeConvectionPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId, 0);
    writer.integerUnlessDefault(3, card.matrixForm, 0);
    writer.realUnlessDefault(4, card.convectionExponent, 0.0);
    if (card.formulaKind == FORMULA_KIND_BY_AMBIENT_POINT)
    {
        writer.integer(5, FORMULA_KIND_BY_AMBIENT_POINT);
        // H2 to H8 default to H1: all eight are written.
        for (int index = 0; index < 8; ++index)
            writer.real(6 + index, card.convectionCoefficient[static_cast<std::size_t>(index)]);
        return;
    }
    writer.integerUnlessDefault(5, card.formulaKind, 0);
    writer.integerUnlessDefault(6, card.tableId, 0);
    if (card.characteristicLength)
        writer.real(9, *card.characteristicLength);
    writer.integerUnlessDefault(10, card.inletNode, 0);
    writer.integerUnlessDefault(11, card.directionFrameId, 0);
    if (card.direction1)
        writer.real(12, *card.direction1);
    if (card.direction2)
        writer.real(13, *card.direction2);
    if (card.direction3)
        writer.real(14, *card.direction3);
}

std::uint32_t FreeConvectionPropTable::append(const FreeConvectionPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_matrixForm.push(card.matrixForm);
    m_convectionExponent.push(card.convectionExponent);
    m_formulaKind.push(card.formulaKind);
    m_tableId.push(card.tableId);
    m_characteristicLength.push(card.characteristicLength.value_or(notGiven()));
    m_inletNode.push(card.inletNode);
    m_directionFrameId.push(card.directionFrameId);
    m_direction1.push(card.direction1.value_or(notGiven()));
    m_direction2.push(card.direction2.value_or(notGiven()));
    m_direction3.push(card.direction3.value_or(notGiven()));
    m_convectionCoefficient.push(card.convectionCoefficient);
    return row;
}

FreeConvectionPropCard FreeConvectionPropTable::card(std::uint32_t row) const
{
    FreeConvectionPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.matrixForm = m_matrixForm.get(row);
    card.convectionExponent = m_convectionExponent.get(row);
    card.formulaKind = m_formulaKind.get(row);
    card.tableId = m_tableId.get(row);
    card.characteristicLength = given(m_characteristicLength.get(row));
    card.inletNode = m_inletNode.get(row);
    card.directionFrameId = m_directionFrameId.get(row);
    card.direction1 = given(m_direction1.get(row));
    card.direction2 = given(m_direction2.get(row));
    card.direction3 = given(m_direction3.get(row));
    card.convectionCoefficient = m_convectionCoefficient.get(row);
    return card;
}

void FreeConvectionPropTable::store(std::uint32_t row, const FreeConvectionPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_matrixForm.set(row, card.matrixForm);
    m_convectionExponent.set(row, card.convectionExponent);
    m_formulaKind.set(row, card.formulaKind);
    m_tableId.set(row, card.tableId);
    m_characteristicLength.set(row, card.characteristicLength.value_or(notGiven()));
    m_inletNode.set(row, card.inletNode);
    m_directionFrameId.set(row, card.directionFrameId);
    m_direction1.set(row, card.direction1.value_or(notGiven()));
    m_direction2.set(row, card.direction2.value_or(notGiven()));
    m_direction3.set(row, card.direction3.value_or(notGiven()));
    m_convectionCoefficient.set(row, card.convectionCoefficient);
}

void FreeConvectionPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_matrixForm.keepRows(rows);
    m_convectionExponent.keepRows(rows);
    m_formulaKind.keepRows(rows);
    m_tableId.keepRows(rows);
    m_characteristicLength.keepRows(rows);
    m_inletNode.keepRows(rows);
    m_directionFrameId.keepRows(rows);
    m_direction1.keepRows(rows);
    m_direction2.keepRows(rows);
    m_direction3.keepRows(rows);
    m_convectionCoefficient.keepRows(rows);
}

void FreeConvectionPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::ThermalMaterial, matId));
    if (const base::EntityId tableId = m_tableId.get(row); tableId > 0)
        m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::HeatTable, tableId));
    if (const base::EntityId inletNode = m_inletNode.get(row); inletNode > 0)
        m_inletNode.set(row, visitor.visit("inletNode", ReferenceTarget::Node, inletNode));
    if (const base::EntityId directionFrameId = m_directionFrameId.get(row); directionFrameId > 0)
        m_directionFrameId.set(row, visitor.visit("directionFrameId", ReferenceTarget::Coordinate, directionFrameId));
}

} // namespace oor::model
