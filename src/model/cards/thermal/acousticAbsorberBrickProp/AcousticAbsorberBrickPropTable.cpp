#include "oor/model/cards/thermal/acousticAbsorberBrickProp/AcousticAbsorberBrickPropTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

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

} // namespace

AcousticAbsorberBrickPropTable::AcousticAbsorberBrickPropTable()
    : m_synthesize("Yes"),
      m_reservedValue(notGiven()),
      m_cutoffFrequency(notGiven()),
      m_damping(notGiven()),
      m_stiffness(notGiven()),
      m_mass(notGiven())
{
}

std::optional<AcousticAbsorberBrickPropCard> AcousticAbsorberBrickPropFormat::read(FieldReader& reader)
{
    AcousticAbsorberBrickPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.synthesize = readKeyword(reader, 2, "synthesize", {"Yes", "No"}, "Yes");
    card.resistanceTableId = reader.integer(3, "resistanceTableId", 0);
    card.reactanceTableId = reader.integer(4, "reactanceTableId", 0);
    card.weightingTableId = reader.integer(5, "weightingTableId", 0);
    card.reservedValue = reader.optionalReal(6, "reservedValue");
    card.cutoffFrequency = reader.optionalReal(7, "cutoffFrequency");
    card.damping = reader.optionalReal(8, "damping");
    card.stiffness = reader.optionalReal(9, "stiffness");
    card.mass = reader.optionalReal(10, "mass");
    requireUnusedBlank(reader, {}, 10);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticAbsorberBrickPropFormat::check(const AcousticAbsorberBrickPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (const auto problem = checkKeyword(card.synthesize, "synthesize", {"Yes", "No"}))
        return problem;
    if (card.resistanceTableId < 0 || card.reactanceTableId < 0 || card.weightingTableId < 0)
        return CardProblem{"card.bad-id", "a table ID cannot be negative", "resistanceTableId"};
    if (card.synthesize == "Yes")
    {
        // the equivalent model is derived from the tables TID1 and TID2 (and TID3, which is optional).
        if (card.resistanceTableId == 0 || card.reactanceTableId == 0)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("reactanceTableId") +
                                   " are required when " + CardProblem::fieldNamed("synthesize") + " is YES",
                               "resistanceTableId"};
        if (!card.cutoffFrequency)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + " is required when " +
                                   CardProblem::fieldNamed("synthesize") + " is YES",
                               "cutoffFrequency"};
    }
    if (card.cutoffFrequency && !(*card.cutoffFrequency > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "cutoffFrequency"};
    if (card.damping && !(*card.damping >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "damping"};
    if (card.stiffness && !(*card.stiffness >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "stiffness"};
    if (card.mass && !(*card.mass >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "mass"};
    return std::nullopt;
}

void AcousticAbsorberBrickPropFormat::write(const AcousticAbsorberBrickPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, card.synthesize, "Yes");
    writer.integerUnlessDefault(3, card.resistanceTableId, 0);
    writer.integerUnlessDefault(4, card.reactanceTableId, 0);
    writer.integerUnlessDefault(5, card.weightingTableId, 0);
    if (card.reservedValue)
        writer.real(6, *card.reservedValue);
    if (card.cutoffFrequency)
        writer.real(7, *card.cutoffFrequency);
    if (card.damping)
        writer.real(8, *card.damping);
    if (card.stiffness)
        writer.real(9, *card.stiffness);
    if (card.mass)
        writer.real(10, *card.mass);
}

std::uint32_t AcousticAbsorberBrickPropTable::append(const AcousticAbsorberBrickPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_synthesize.push(card.synthesize);
    m_resistanceTableId.push(card.resistanceTableId);
    m_reactanceTableId.push(card.reactanceTableId);
    m_weightingTableId.push(card.weightingTableId);
    m_reservedValue.push(card.reservedValue.value_or(notGiven()));
    m_cutoffFrequency.push(card.cutoffFrequency.value_or(notGiven()));
    m_damping.push(card.damping.value_or(notGiven()));
    m_stiffness.push(card.stiffness.value_or(notGiven()));
    m_mass.push(card.mass.value_or(notGiven()));
    return row;
}

AcousticAbsorberBrickPropCard AcousticAbsorberBrickPropTable::card(std::uint32_t row) const
{
    AcousticAbsorberBrickPropCard card;
    card.id = m_id.get(row);
    card.synthesize = m_synthesize.get(row);
    card.resistanceTableId = m_resistanceTableId.get(row);
    card.reactanceTableId = m_reactanceTableId.get(row);
    card.weightingTableId = m_weightingTableId.get(row);
    card.reservedValue = given(m_reservedValue.get(row));
    card.cutoffFrequency = given(m_cutoffFrequency.get(row));
    card.damping = given(m_damping.get(row));
    card.stiffness = given(m_stiffness.get(row));
    card.mass = given(m_mass.get(row));
    return card;
}

void AcousticAbsorberBrickPropTable::store(std::uint32_t row, const AcousticAbsorberBrickPropCard& card)
{
    m_id.set(row, card.id);
    m_synthesize.set(row, card.synthesize);
    m_resistanceTableId.set(row, card.resistanceTableId);
    m_reactanceTableId.set(row, card.reactanceTableId);
    m_weightingTableId.set(row, card.weightingTableId);
    m_reservedValue.set(row, card.reservedValue.value_or(notGiven()));
    m_cutoffFrequency.set(row, card.cutoffFrequency.value_or(notGiven()));
    m_damping.set(row, card.damping.value_or(notGiven()));
    m_stiffness.set(row, card.stiffness.value_or(notGiven()));
    m_mass.set(row, card.mass.value_or(notGiven()));
}

void AcousticAbsorberBrickPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_synthesize.keepRows(rows);
    m_resistanceTableId.keepRows(rows);
    m_reactanceTableId.keepRows(rows);
    m_weightingTableId.keepRows(rows);
    m_reservedValue.keepRows(rows);
    m_cutoffFrequency.keepRows(rows);
    m_damping.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_mass.keepRows(rows);
}

void AcousticAbsorberBrickPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId resistanceTableId = m_resistanceTableId.get(row); resistanceTableId > 0)
        m_resistanceTableId.set(row,
                                visitor.visit("resistanceTableId", ReferenceTarget::DynamicTable, resistanceTableId));
    if (const base::EntityId reactanceTableId = m_reactanceTableId.get(row); reactanceTableId > 0)
        m_reactanceTableId.set(row, visitor.visit("reactanceTableId", ReferenceTarget::DynamicTable, reactanceTableId));
    if (const base::EntityId weightingTableId = m_weightingTableId.get(row); weightingTableId > 0)
        m_weightingTableId.set(row, visitor.visit("weightingTableId", ReferenceTarget::DynamicTable, weightingTableId));
}

} // namespace oor::model
