#include "oor/model/cards/explicit/inflatorGas/InflatorGasTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// V2, V3 and V4 belong to the polynomial: with another law they have no value. (A card read from a deck keeps the
// numbers it finds, so a value in these fields of a CONSTANT or TABLE card is reported by name.)
std::optional<CardProblem> checkUnusedCoefficients(const InflatorGasCard& card)
{
    if (card.heatCoefficient2 != 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is used only with " +
                               CardProblem::fieldNamed("heatLaw") + " = POLY and must be blank",
                           "heatCoefficient2"};
    if (card.heatCoefficient3 != 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is used only with " +
                               CardProblem::fieldNamed("heatLaw") + " = POLY and must be blank",
                           "heatCoefficient3"};
    if (card.heatCoefficient4 != 0.0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is used only with " +
                               CardProblem::fieldNamed("heatLaw") + " = POLY and must be blank",
                           "heatCoefficient4"};
    return std::nullopt;
}

} // namespace

std::optional<InflatorGasCard> InflatorGasFormat::read(FieldReader& reader)
{
    InflatorGasCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type =
        readKeyword(reader, 2, "constantKind", INFLATOR_GAS_CONSTANT_WORDS, InflatorGasConstant::SpecificGasConstant);
    card.constantValue = reader.requiredReal(3, "constantValue");
    card.heatLaw = readKeyword(reader, 4, "heatLaw", INFLATOR_GAS_HEAT_LAW_WORDS, InflatorGasHeatLaw::Constant);
    if (card.heatLaw == InflatorGasHeatLaw::Table)
        card.heatTableId = reader.requiredInteger(5, "heatCoefficient1");
    else
        card.heatCoefficient1 = reader.requiredReal(5, "heatCoefficient1");
    card.heatCoefficient2 = reader.real(6, "heatCoefficient2", 0.0);
    card.heatCoefficient3 = reader.real(7, "heatCoefficient3", 0.0);
    card.heatCoefficient4 = reader.real(8, "heatCoefficient4", 0.0);
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InflatorGasFormat::check(const InflatorGasCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.constantValue, "constantValue", RealRule::Positive))
        return problem;
    if (card.heatLaw == InflatorGasHeatLaw::Table)
    {
        if (const auto problem = checkRequiredId(card.heatTableId, "heatCoefficient1"))
            return problem;
    }
    else if (const auto problem = checkReal(card.heatCoefficient1, "heatCoefficient1", RealRule::Positive))
    {
        return problem;
    }
    if (const auto problem = checkReal(card.heatCoefficient2, "heatCoefficient2", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.heatCoefficient3, "heatCoefficient3", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.heatCoefficient4, "heatCoefficient4", RealRule::Finite))
        return problem;
    if (card.heatLaw != InflatorGasHeatLaw::Polynomial)
        return checkUnusedCoefficients(card);
    return std::nullopt;
}

void InflatorGasFormat::write(const InflatorGasCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, INFLATOR_GAS_CONSTANT_WORDS, card.type,
                              InflatorGasConstant::SpecificGasConstant);
    writer.real(3, card.constantValue);
    writeKeywordUnlessDefault(writer, 4, INFLATOR_GAS_HEAT_LAW_WORDS, card.heatLaw, InflatorGasHeatLaw::Constant);
    if (card.heatLaw == InflatorGasHeatLaw::Table)
        writer.integer(5, card.heatTableId);
    else
        writer.real(5, card.heatCoefficient1);
    writer.realUnlessDefault(6, card.heatCoefficient2, 0.0);
    writer.realUnlessDefault(7, card.heatCoefficient3, 0.0);
    writer.realUnlessDefault(8, card.heatCoefficient4, 0.0);
}

std::uint32_t InflatorGasTable::append(const InflatorGasCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InflatorGasTable::store(std::uint32_t row, const InflatorGasCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InflatorGasTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InflatorGasTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InflatorGasTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    InflatorGasCard& card = m_cards.edit(row);
    if (card.heatLaw == InflatorGasHeatLaw::Table)
        visitId(visitor, "heatCoefficient1", ReferenceTarget::DynamicTable, card.heatTableId);
}

} // namespace oor::model
