#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsTable.hpp"

#include <string>

namespace oor::model
{

std::optional<HeatSurfaceSideCard> HeatSurfaceSideFormat::read(FieldReader& reader)
{
    HeatSurfaceSideCard card;
    card.id = reader.requiredInteger(1, "id");
    card.conductionElemId = reader.requiredInteger(2, "conductionElemId");
    card.sideCode = reader.requiredInteger(3, "sideCode");
    card.frontViewId = reader.integer(4, "frontViewId", 0);
    card.backViewId = reader.integer(5, "backViewId", 0);
    card.frontRadiationPropId = reader.integer(6, "frontRadiationPropId", 0);
    card.backRadiationPropId = reader.integer(7, "backRadiationPropId", 0);
    requireUnusedBlank(reader, {}, 7);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatSurfaceSideFormat::check(const HeatSurfaceSideCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.conductionElemId <= 0)
        return CardProblem{"card.bad-id", "the conduction element ID must be a positive integer", "conductionElemId"};
    if (card.sideCode < 1 || card.sideCode > 6)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be between 1 and 6", "sideCode"};
    if (card.frontViewId < 0 || card.backViewId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "frontViewId"};
    if (card.frontRadiationPropId < 0 || card.backRadiationPropId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative",
                           "frontRadiationPropId"};
    return std::nullopt;
}

void HeatSurfaceSideFormat::write(const HeatSurfaceSideCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.conductionElemId);
    writer.integer(3, card.sideCode);
    writer.integerUnlessDefault(4, card.frontViewId, 0);
    writer.integerUnlessDefault(5, card.backViewId, 0);
    writer.integerUnlessDefault(6, card.frontRadiationPropId, 0);
    writer.integerUnlessDefault(7, card.backRadiationPropId, 0);
}

std::uint32_t HeatSurfaceSideTable::append(const HeatSurfaceSideCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_conductionElemId.push(card.conductionElemId);
    m_sideCode.push(card.sideCode);
    m_frontViewId.push(card.frontViewId);
    m_backViewId.push(card.backViewId);
    m_frontRadiationPropId.push(card.frontRadiationPropId);
    m_backRadiationPropId.push(card.backRadiationPropId);
    return row;
}

HeatSurfaceSideCard HeatSurfaceSideTable::card(std::uint32_t row) const
{
    HeatSurfaceSideCard card;
    card.id = m_id.get(row);
    card.conductionElemId = m_conductionElemId.get(row);
    card.sideCode = m_sideCode.get(row);
    card.frontViewId = m_frontViewId.get(row);
    card.backViewId = m_backViewId.get(row);
    card.frontRadiationPropId = m_frontRadiationPropId.get(row);
    card.backRadiationPropId = m_backRadiationPropId.get(row);
    return card;
}

void HeatSurfaceSideTable::store(std::uint32_t row, const HeatSurfaceSideCard& card)
{
    m_id.set(row, card.id);
    m_conductionElemId.set(row, card.conductionElemId);
    m_sideCode.set(row, card.sideCode);
    m_frontViewId.set(row, card.frontViewId);
    m_backViewId.set(row, card.backViewId);
    m_frontRadiationPropId.set(row, card.frontRadiationPropId);
    m_backRadiationPropId.set(row, card.backRadiationPropId);
}

void HeatSurfaceSideTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_conductionElemId.keepRows(rows);
    m_sideCode.keepRows(rows);
    m_frontViewId.keepRows(rows);
    m_backViewId.keepRows(rows);
    m_frontRadiationPropId.keepRows(rows);
    m_backRadiationPropId.keepRows(rows);
}

void HeatSurfaceSideTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_conductionElemId.set(row,
                           visitor.visit("conductionElemId", ReferenceTarget::Element, m_conductionElemId.get(row)));
    if (const base::EntityId frontViewId = m_frontViewId.get(row); frontViewId > 0)
        m_frontViewId.set(row, visitor.visit("frontViewId", ReferenceTarget::ViewFactorFaceFactor, frontViewId));
    if (const base::EntityId backViewId = m_backViewId.get(row); backViewId > 0)
        m_backViewId.set(row, visitor.visit("backViewId", ReferenceTarget::ViewFactorFaceFactor, backViewId));
    if (const base::EntityId frontRadiationPropId = m_frontRadiationPropId.get(row); frontRadiationPropId > 0)
        m_frontRadiationPropId.set(
            row, visitor.visit("frontRadiationPropId", ReferenceTarget::RadiationMaterial, frontRadiationPropId));
    if (const base::EntityId backRadiationPropId = m_backRadiationPropId.get(row); backRadiationPropId > 0)
        m_backRadiationPropId.set(
            row, visitor.visit("backRadiationPropId", ReferenceTarget::RadiationMaterial, backRadiationPropId));
}

void HeatSurfaceSideTable::finishRead(Model& model, const ReadContext&)
{
    const std::optional<HeatSurfaceDefaultsCard> defaults = heatSurfaceDefaultsDefaults(model);
    if (!defaults)
        return;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        if (m_frontViewId.get(row) == 0)
            m_frontViewId.set(row, defaults->frontViewId);
        if (m_backViewId.get(row) == 0)
            m_backViewId.set(row, defaults->backViewId);
        if (m_frontRadiationPropId.get(row) == 0)
            m_frontRadiationPropId.set(row, defaults->frontRadiationPropId);
        if (m_backRadiationPropId.get(row) == 0)
            m_backRadiationPropId.set(row, defaults->backRadiationPropId);
    }
}

} // namespace oor::model
