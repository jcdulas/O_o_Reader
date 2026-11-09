#include "oor/model/cards/optimisation/designControlSurface/DesignControlSurfaceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

namespace oor::model
{

std::optional<DesignControlSurfaceCard> DesignControlSurfaceFormat::read(FieldReader& reader)
{
    DesignControlSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.controlSurfaceLabel = std::string(reader.text(2));
    if (card.controlSurfaceLabel.empty())
        reader.error("card.missing-field", std::string("controlSurfaceLabel") + " is required", "controlSurfaceLabel");
    card.trimId = reader.requiredInteger(3, "trimId");
    card.designVarId = reader.requiredInteger(4, "designVarId");
    card.coefficient = reader.requiredReal(5, "coefficient");
    requireBlankFields(reader, {}, 5);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignControlSurfaceFormat::check(const DesignControlSurfaceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // The label of an AESURF has up to eight characters and must read back from every format, like the other labels of
    // the design cards.
    if (const auto problem = checkDesignLabel(card.controlSurfaceLabel, "controlSurfaceLabel"))
        return problem;
    if (const auto problem = checkRequiredId(card.trimId, "trimId"))
        return problem;
    if (const auto problem = checkRequiredId(card.designVarId, "designVarId"))
        return problem;
    return checkReal(card.coefficient, "coefficient", RealRule::Finite);
}

void DesignControlSurfaceFormat::write(const DesignControlSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.controlSurfaceLabel);
    writer.integer(3, card.trimId);
    writer.integer(4, card.designVarId);
    writer.real(5, card.coefficient);
}

std::uint32_t DesignControlSurfaceTable::append(const DesignControlSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_controlSurfaceLabel.push(card.controlSurfaceLabel);
    m_trimId.push(card.trimId);
    m_designVarId.push(card.designVarId);
    m_coefficient.push(card.coefficient);
    return row;
}

DesignControlSurfaceCard DesignControlSurfaceTable::card(std::uint32_t row) const
{
    DesignControlSurfaceCard card;
    card.id = m_id.get(row);
    card.controlSurfaceLabel = m_controlSurfaceLabel.get(row);
    card.trimId = m_trimId.get(row);
    card.designVarId = m_designVarId.get(row);
    card.coefficient = m_coefficient.get(row);
    return card;
}

void DesignControlSurfaceTable::store(std::uint32_t row, const DesignControlSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_controlSurfaceLabel.set(row, card.controlSurfaceLabel);
    m_trimId.set(row, card.trimId);
    m_designVarId.set(row, card.designVarId);
    m_coefficient.set(row, card.coefficient);
}

void DesignControlSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_controlSurfaceLabel.keepRows(rows);
    m_trimId.keepRows(rows);
    m_designVarId.keepRows(rows);
    m_coefficient.keepRows(rows);
}

void DesignControlSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The AELABEL is a label, not an ID: it is matched by name.
    base::EntityId trimId = m_trimId.get(row);
    visitId(visitor, "trimId", ReferenceTarget::AeroTrimSet, trimId);
    m_trimId.set(row, trimId);
    base::EntityId designVarId = m_designVarId.get(row);
    visitId(visitor, "designVarId", ReferenceTarget::DesignVariable, designVarId);
    m_designVarId.set(row, designVarId);
}

} // namespace oor::model
