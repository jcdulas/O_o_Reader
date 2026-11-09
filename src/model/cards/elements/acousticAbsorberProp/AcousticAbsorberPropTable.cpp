#include "oor/model/cards/elements/acousticAbsorberProp/AcousticAbsorberPropTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

AcousticAbsorberPropTable::AcousticAbsorberPropTable()
    : m_impedanceScale(1.0), m_area(1.0), m_characteristicImpedance(1.0)
{
}

std::optional<AcousticAbsorberPropCard> AcousticAbsorberPropFormat::read(FieldReader& reader)
{
    AcousticAbsorberPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.resistanceTableId = reader.integer(2, "resistanceTableId", 0);
    card.reactanceTableId = reader.integer(3, "reactanceTableId", 0);
    card.impedanceScale = reader.real(4, "impedanceScale", 1.0);
    card.area = reader.real(5, "area", 1.0);
    card.damping = reader.real(6, "damping", 0.0);
    card.stiffness = reader.real(7, "stiffness", 0.0);
    card.characteristicImpedance = reader.real(8, "characteristicImpedance", 1.0);
    requireUnusedBlank(reader, {}, 8);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticAbsorberPropFormat::check(const AcousticAbsorberPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.resistanceTableId < 0 || card.reactanceTableId < 0)
        return CardProblem{"card.bad-id", "a table ID cannot be negative", "resistanceTableId"};
    if (!(card.area > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "area"};
    if (!(card.damping >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "damping"};
    if (!(card.stiffness >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "stiffness"};
    // at least one of the four fields that give the impedance must be given.
    if (card.resistanceTableId == 0 && card.reactanceTableId == 0 && card.damping == 0.0 && card.stiffness == 0.0)
        return CardProblem{"card.missing-field",
                           "at least one of " + CardProblem::fieldNamed("resistanceTableId") + ", " +
                               CardProblem::fieldNamed("reactanceTableId") + ", " + CardProblem::fieldNamed("damping") +
                               " and " + CardProblem::fieldNamed("stiffness") + " must be given",
                           "resistanceTableId"};
    return std::nullopt;
}

void AcousticAbsorberPropFormat::write(const AcousticAbsorberPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.resistanceTableId, 0);
    writer.integerUnlessDefault(3, card.reactanceTableId, 0);
    writer.realUnlessDefault(4, card.impedanceScale, 1.0);
    writer.realUnlessDefault(5, card.area, 1.0);
    writer.realUnlessDefault(6, card.damping, 0.0);
    writer.realUnlessDefault(7, card.stiffness, 0.0);
    writer.realUnlessDefault(8, card.characteristicImpedance, 1.0);
}

std::uint32_t AcousticAbsorberPropTable::append(const AcousticAbsorberPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_resistanceTableId.push(card.resistanceTableId);
    m_reactanceTableId.push(card.reactanceTableId);
    m_impedanceScale.push(card.impedanceScale);
    m_area.push(card.area);
    m_damping.push(card.damping);
    m_stiffness.push(card.stiffness);
    m_characteristicImpedance.push(card.characteristicImpedance);
    return row;
}

AcousticAbsorberPropCard AcousticAbsorberPropTable::card(std::uint32_t row) const
{
    AcousticAbsorberPropCard card;
    card.id = m_id.get(row);
    card.resistanceTableId = m_resistanceTableId.get(row);
    card.reactanceTableId = m_reactanceTableId.get(row);
    card.impedanceScale = m_impedanceScale.get(row);
    card.area = m_area.get(row);
    card.damping = m_damping.get(row);
    card.stiffness = m_stiffness.get(row);
    card.characteristicImpedance = m_characteristicImpedance.get(row);
    return card;
}

void AcousticAbsorberPropTable::store(std::uint32_t row, const AcousticAbsorberPropCard& card)
{
    m_id.set(row, card.id);
    m_resistanceTableId.set(row, card.resistanceTableId);
    m_reactanceTableId.set(row, card.reactanceTableId);
    m_impedanceScale.set(row, card.impedanceScale);
    m_area.set(row, card.area);
    m_damping.set(row, card.damping);
    m_stiffness.set(row, card.stiffness);
    m_characteristicImpedance.set(row, card.characteristicImpedance);
}

void AcousticAbsorberPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_resistanceTableId.keepRows(rows);
    m_reactanceTableId.keepRows(rows);
    m_impedanceScale.keepRows(rows);
    m_area.keepRows(rows);
    m_damping.keepRows(rows);
    m_stiffness.keepRows(rows);
    m_characteristicImpedance.keepRows(rows);
}

void AcousticAbsorberPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId resistanceTableId = m_resistanceTableId.get(row); resistanceTableId > 0)
        m_resistanceTableId.set(row,
                                visitor.visit("resistanceTableId", ReferenceTarget::DynamicTable, resistanceTableId));
    if (const base::EntityId reactanceTableId = m_reactanceTableId.get(row); reactanceTableId > 0)
        m_reactanceTableId.set(row, visitor.visit("reactanceTableId", ReferenceTarget::DynamicTable, reactanceTableId));
}

} // namespace oor::model
