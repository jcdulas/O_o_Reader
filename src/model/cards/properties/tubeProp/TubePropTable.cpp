#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"

namespace oor::model
{

std::optional<TubePropCard> TubePropFormat::read(FieldReader& reader)
{
    TubePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.outerDiameter = reader.requiredReal(3, "outerDiameter");
    // A blank T is a solid rod (half the diameter), a blank OD2 is OD. Stored resolved.
    card.thicknessGiven = !reader.isBlank(4);
    card.thickness = reader.real(4, "thickness", 0.5 * card.outerDiameter);
    card.nonStructMass = reader.real(5, "nonStructMass", 0.0);
    card.outerDiameter2Given = !reader.isBlank(6);
    card.outerDiameter2 = reader.real(6, "outerDiameter2", card.outerDiameter);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TubePropFormat::check(const TubePropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (!(card.outerDiameter > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "outerDiameter"};
    if (card.thickness < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "thickness"};
    if (!(card.outerDiameter2 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "outerDiameter2"};
    return std::nullopt;
}

void TubePropFormat::write(const TubePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.outerDiameter);
    if (card.thicknessGiven) // a blank is OD/2: written blank, so that it follows OD when read again
        writer.real(4, card.thickness);
    writer.realUnlessDefault(5, card.nonStructMass, 0.0);
    if (card.outerDiameter2Given) // a blank is OD: written blank
        writer.real(6, card.outerDiameter2);
}

std::uint32_t TubePropTable::append(const TubePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_outerDiameter.push(card.outerDiameter);
    m_thickness.push(card.thickness);
    m_nonStructMass.push(card.nonStructMass);
    m_outerDiameter2.push(card.outerDiameter2);
    m_thicknessGiven.push(card.thicknessGiven ? 1 : 0);
    m_outerDiameter2Given.push(card.outerDiameter2Given ? 1 : 0);
    return row;
}

TubePropCard TubePropTable::card(std::uint32_t row) const
{
    return TubePropCard{m_id.get(row),
                        m_matId.get(row),
                        m_outerDiameter.get(row),
                        m_thickness.get(row),
                        m_nonStructMass.get(row),
                        m_outerDiameter2.get(row),
                        m_thicknessGiven.get(row) != 0,
                        m_outerDiameter2Given.get(row) != 0};
}

void TubePropTable::store(std::uint32_t row, const TubePropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_outerDiameter.set(row, card.outerDiameter);
    m_thickness.set(row, card.thickness);
    m_nonStructMass.set(row, card.nonStructMass);
    m_outerDiameter2.set(row, card.outerDiameter2);
    m_thicknessGiven.set(row, card.thicknessGiven ? 1 : 0);
    m_outerDiameter2Given.set(row, card.outerDiameter2Given ? 1 : 0);
}

void TubePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_outerDiameter.keepRows(rows);
    m_thickness.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_outerDiameter2.keepRows(rows);
    m_thicknessGiven.keepRows(rows);
    m_outerDiameter2Given.keepRows(rows);
}

void TubePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
