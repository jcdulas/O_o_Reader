#include "oor/model/cards/explicit/porousTimeFlowBoundary/PorousTimeFlowBoundaryTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// When the inflowing material is given (MID), its density and energy must be given as well: the keyword, and
// the number of a constant (a table has its ID and ELEMENT takes no value, which readSourcedValue already checks).
void requireGivenWithMaterial(FieldReader& reader, const SourcedValueLayout& layout, const SourcedValue& value)
{
    if (reader.isBlank(layout.sourceField))
    {
        reader.error("card.missing-field",
                     std::string(layout.sourceName) + " is required when " + std::string("matId") + " is given",
                     layout.sourceName);
        return;
    }
    if (value.source == ValueSource::Constant && reader.isBlank(layout.valueField))
        reader.error("card.missing-field",
                     std::string(layout.valueName) + " is required when " + std::string("matId") + " is given",
                     layout.valueName);
}

// A quantity written whole, keyword and value, where a blank would not read back as it was (the density and energy of a
// card that has a MID).
void writeQuantityInFull(FieldWriter& writer, const SourcedValueLayout& layout, const SourcedValue& value)
{
    writeKeyword(writer, layout.sourceField, VALUE_SOURCE_WORDS, value.source);
    switch (value.source)
    {
    case ValueSource::Element:
        break;
    case ValueSource::Constant:
        writer.real(layout.valueField, value.constant);
        break;
    case ValueSource::Table:
        writer.integer(layout.valueField, value.table);
        break;
    }
}

} // namespace

std::optional<PorousTimeFlowBoundaryCard> PorousTimeFlowBoundaryFormat::read(FieldReader& reader)
{
    PorousTimeFlowBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "flowDirection", FLOW_DIRECTION_WORDS, FlowDirection::Both);
    card.velocity = readSourcedValue(reader, POROUS_TIME_FLOW_VELOCITY_LAYOUT);
    card.pressure = readSourcedValue(reader, POROUS_TIME_FLOW_PRESSURE_LAYOUT);
    card.matId = reader.integer(17, "matId", 0);
    card.density = readSourcedValue(reader, POROUS_TIME_FLOW_DENSITY_LAYOUT);
    card.energy = readSourcedValue(reader, POROUS_TIME_FLOW_ENERGY_LAYOUT);
    // Fields 3 to 8 and 13 to 16 are empty cells of the grid, and nothing follows field 21.
    requireOnlyFields(reader, {1, 2, 9, 10, 11, 12, 17, 18, 19, 20, 21});
    if (card.matId > 0)
    {
        requireGivenWithMaterial(reader, POROUS_TIME_FLOW_DENSITY_LAYOUT, card.density);
        requireGivenWithMaterial(reader, POROUS_TIME_FLOW_ENERGY_LAYOUT, card.energy);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PorousTimeFlowBoundaryFormat::check(const PorousTimeFlowBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkSourcedValue(card.velocity, POROUS_TIME_FLOW_VELOCITY_LAYOUT, RealRule::Finite))
        return problem;
    if (const auto problem = checkSourcedValue(card.pressure, POROUS_TIME_FLOW_PRESSURE_LAYOUT, RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    // An outflow boundary uses the material of the adjacent Euler element: no inflowing material.
    if (card.type == FlowDirection::Out && card.matId > 0)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " is not allowed when " +
                               CardProblem::fieldNamed("flowDirection") + " is OUT",
                           "matId"};
    if (const auto problem = checkSourcedValue(card.density, POROUS_TIME_FLOW_DENSITY_LAYOUT, RealRule::Finite))
        return problem;
    return checkSourcedValue(card.energy, POROUS_TIME_FLOW_ENERGY_LAYOUT, RealRule::Finite);
}

void PorousTimeFlowBoundaryFormat::write(const PorousTimeFlowBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, FLOW_DIRECTION_WORDS, card.type, FlowDirection::Both);
    writeSourcedValue(writer, POROUS_TIME_FLOW_VELOCITY_LAYOUT, card.velocity);
    writeSourcedValue(writer, POROUS_TIME_FLOW_PRESSURE_LAYOUT, card.pressure);
    writer.integerUnlessDefault(17, card.matId, 0);
    if (card.matId > 0)
    {
        writeQuantityInFull(writer, POROUS_TIME_FLOW_DENSITY_LAYOUT, card.density);
        writeQuantityInFull(writer, POROUS_TIME_FLOW_ENERGY_LAYOUT, card.energy);
    }
    else
    {
        writeSourcedValue(writer, POROUS_TIME_FLOW_DENSITY_LAYOUT, card.density);
        writeSourcedValue(writer, POROUS_TIME_FLOW_ENERGY_LAYOUT, card.energy);
    }
}

std::uint32_t PorousTimeFlowBoundaryTable::append(const PorousTimeFlowBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PorousTimeFlowBoundaryTable::store(std::uint32_t row, const PorousTimeFlowBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PorousTimeFlowBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PorousTimeFlowBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PorousTimeFlowBoundaryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    PorousTimeFlowBoundaryCard& card = m_cards.edit(row);
    visitSourcedTable(visitor, "velocity", card.velocity);
    visitSourcedTable(visitor, "pressure", card.pressure);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    visitSourcedTable(visitor, "density", card.density);
    visitSourcedTable(visitor, "specificEnergy", card.energy);
}

} // namespace oor::model
