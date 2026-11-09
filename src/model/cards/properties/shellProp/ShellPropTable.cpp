#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

double blankIfNone(const std::optional<double>& value)
{
    return value ? *value : std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> noneIfBlank(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

} // namespace

std::optional<ShellPropCard> ShellPropFormat::read(FieldReader& reader)
{
    ShellPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.membraneMatId = reader.integer(2, "membraneMatId", 0);
    card.thickness = reader.real(3, "thickness", 0.0);
    card.bendingMatId = reader.integer(4, "bendingMatId", 0);
    card.bendingStiffnessRatio = reader.real(5, "bendingStiffnessRatio", 1.0);
    card.shearMatId = reader.integer(6, "shearMatId", 0);
    card.shearThicknessRatio = reader.real(7, "shearThicknessRatio", 0.833333);
    card.nonStructMass = reader.real(8, "nonStructMass", 0.0);
    // Z1 and Z2 default to -T/2 and +T/2 of the element's own thickness; they stay blank.
    card.fiberDistance1 = reader.optionalReal(9, "fiberDistance1");
    card.fiberDistance2 = reader.optionalReal(10, "fiberDistance2");
    card.couplingMatId = reader.integer(11, "couplingMatId", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellPropFormat::check(const ShellPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.thickness < 0.0)
        return CardProblem{"card.bad-value", "T must not be negative", "thickness"};
    return std::nullopt;
}

void ShellPropFormat::write(const ShellPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.membraneMatId, 0);
    writer.real(3, card.thickness);
    writer.integerUnlessDefault(4, card.bendingMatId, 0);
    writer.realUnlessDefault(5, card.bendingStiffnessRatio, 1.0);
    writer.integerUnlessDefault(6, card.shearMatId, 0);
    writer.realUnlessDefault(7, card.shearThicknessRatio, 0.833333);
    writer.realUnlessDefault(8, card.nonStructMass, 0.0);
    if (card.fiberDistance1)
        writer.real(9, *card.fiberDistance1);
    if (card.fiberDistance2)
        writer.real(10, *card.fiberDistance2);
    writer.integerUnlessDefault(11, card.couplingMatId, 0);
}

std::uint32_t ShellPropTable::append(const ShellPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_membraneMatId.push(card.membraneMatId);
    m_thickness.push(card.thickness);
    m_bendingMatId.push(card.bendingMatId);
    m_bendingStiffnessRatio.push(card.bendingStiffnessRatio);
    m_shearMatId.push(card.shearMatId);
    m_shearThicknessRatio.push(card.shearThicknessRatio);
    m_nonStructMass.push(card.nonStructMass);
    m_fiberDistance1.push(blankIfNone(card.fiberDistance1));
    m_fiberDistance2.push(blankIfNone(card.fiberDistance2));
    m_couplingMatId.push(card.couplingMatId);
    return row;
}

ShellPropCard ShellPropTable::card(std::uint32_t row) const
{
    return ShellPropCard{m_id.get(row),
                         m_membraneMatId.get(row),
                         m_thickness.get(row),
                         m_bendingMatId.get(row),
                         m_bendingStiffnessRatio.get(row),
                         m_shearMatId.get(row),
                         m_shearThicknessRatio.get(row),
                         m_nonStructMass.get(row),
                         noneIfBlank(m_fiberDistance1.get(row)),
                         noneIfBlank(m_fiberDistance2.get(row)),
                         m_couplingMatId.get(row)};
}

void ShellPropTable::store(std::uint32_t row, const ShellPropCard& card)
{
    m_id.set(row, card.id);
    m_membraneMatId.set(row, card.membraneMatId);
    m_thickness.set(row, card.thickness);
    m_bendingMatId.set(row, card.bendingMatId);
    m_bendingStiffnessRatio.set(row, card.bendingStiffnessRatio);
    m_shearMatId.set(row, card.shearMatId);
    m_shearThicknessRatio.set(row, card.shearThicknessRatio);
    m_nonStructMass.set(row, card.nonStructMass);
    m_fiberDistance1.set(row, blankIfNone(card.fiberDistance1));
    m_fiberDistance2.set(row, blankIfNone(card.fiberDistance2));
    m_couplingMatId.set(row, card.couplingMatId);
}

void ShellPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_membraneMatId.keepRows(rows);
    m_thickness.keepRows(rows);
    m_bendingMatId.keepRows(rows);
    m_bendingStiffnessRatio.keepRows(rows);
    m_shearMatId.keepRows(rows);
    m_shearThicknessRatio.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_fiberDistance1.keepRows(rows);
    m_fiberDistance2.keepRows(rows);
    m_couplingMatId.keepRows(rows);
}

void ShellPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto visit = [&](Column<base::EntityId>& column, const char* field)
    {
        if (const base::EntityId id = column.get(row); id > 0)
            column.set(row, visitor.visit(field, ReferenceTarget::Material, id));
    };
    visit(m_membraneMatId, "membraneMatId");
    visit(m_bendingMatId, "bendingMatId");
    visit(m_shearMatId, "shearMatId");
    visit(m_couplingMatId, "couplingMatId");
}

} // namespace oor::model
