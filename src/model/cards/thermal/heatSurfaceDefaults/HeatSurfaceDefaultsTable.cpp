#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

std::optional<HeatSurfaceDefaultsCard> HeatSurfaceDefaultsFormat::read(FieldReader& reader)
{
    HeatSurfaceDefaultsCard card;
    card.kind = readOptionalKeyword<HeatSurfaceDefaultsSurfaceType>(reader, 1, "kind", HEAT_SURFACE_TYPE_WORDS);
    card.frontViewId = reader.integer(2, "frontViewId", 0);
    card.backViewId = reader.integer(3, "backViewId", 0);
    card.frontRadiationPropId = reader.integer(4, "frontRadiationPropId", 0);
    card.backRadiationPropId = reader.integer(5, "backRadiationPropId", 0);
    card.propId = reader.integer(7, "propId", 0);
    card.orientationNode = reader.integer(8, "orientationNode", 0);
    card.directionFrameId = reader.integer(9, "directionFrameId", 0);
    card.direction1 = reader.optionalReal(10, "direction1");
    card.direction2 = reader.optionalReal(11, "direction2");
    card.direction3 = reader.optionalReal(12, "direction3");
    requireUnusedBlank(reader, {6}, 12); // the cell between RADMIDB and PID, and the end of the card
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatSurfaceDefaultsFormat::check(const HeatSurfaceDefaultsCard& card)
{
    if (const auto problem = checkOptionalId(card.frontViewId, "frontViewId"))
        return problem;
    if (const auto problem = checkOptionalId(card.backViewId, "backViewId"))
        return problem;
    if (const auto problem = checkOptionalId(card.frontRadiationPropId, "frontRadiationPropId"))
        return problem;
    if (const auto problem = checkOptionalId(card.backRadiationPropId, "backRadiationPropId"))
        return problem;
    if (const auto problem = checkOptionalId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.orientationNode, "orientationNode"))
        return problem;
    if (const auto problem = checkOptionalId(card.directionFrameId, "directionFrameId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.direction1, "direction1", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.direction2, "direction2", RealRule::Finite))
        return problem;
    return checkOptionalReal(card.direction3, "direction3", RealRule::Finite);
}

void HeatSurfaceDefaultsFormat::write(const HeatSurfaceDefaultsCard& card, FieldWriter& writer)
{
    writeOptionalKeyword(writer, 1, HEAT_SURFACE_TYPE_WORDS, card.kind);
    writer.integerUnlessDefault(2, card.frontViewId, 0);
    writer.integerUnlessDefault(3, card.backViewId, 0);
    writer.integerUnlessDefault(4, card.frontRadiationPropId, 0);
    writer.integerUnlessDefault(5, card.backRadiationPropId, 0);
    writer.integerUnlessDefault(7, card.propId, 0);
    writer.integerUnlessDefault(8, card.orientationNode, 0);
    writer.integerUnlessDefault(9, card.directionFrameId, 0);
    if (card.direction1)
        writer.real(10, *card.direction1);
    if (card.direction2)
        writer.real(11, *card.direction2);
    if (card.direction3)
        writer.real(12, *card.direction3);
}

std::uint32_t HeatSurfaceDefaultsTable::append(const HeatSurfaceDefaultsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_propId.push(card.propId);
    m_cards.push(card);
    return row;
}

void HeatSurfaceDefaultsTable::store(std::uint32_t row, const HeatSurfaceDefaultsCard& card)
{
    m_propId.set(row, card.propId);
    m_cards.set(row, card);
}

void HeatSurfaceDefaultsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_propId.keepRows(rows);
    m_cards.keepRows(rows);
}

void HeatSurfaceDefaultsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    HeatSurfaceDefaultsCard& card = m_cards.edit(row);
    visitId(visitor, "frontViewId", ReferenceTarget::ViewFactorFaceFactor, card.frontViewId);
    visitId(visitor, "backViewId", ReferenceTarget::ViewFactorFaceFactor, card.backViewId);
    visitId(visitor, "frontRadiationPropId", ReferenceTarget::RadiationMaterial, card.frontRadiationPropId);
    visitId(visitor, "backRadiationPropId", ReferenceTarget::RadiationMaterial, card.backRadiationPropId);
    visitId(visitor, "propId", ReferenceTarget::HeatBoundaryProperty, card.propId);
    visitId(visitor, "orientationNode", ReferenceTarget::Node, card.orientationNode);
    visitId(visitor, "directionFrameId", ReferenceTarget::Coordinate,
            card.directionFrameId); // 0 is the basic system: not visited
    m_propId.set(row, card.propId);
}

std::optional<HeatSurfaceDefaultsCard> heatSurfaceDefaultsDefaults(const Model& model)
{
    const HeatSurfaceDefaultsTable* table = model.findTable<HeatSurfaceDefaultsTable>();
    if (table == nullptr)
        return std::nullopt;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (table->rows().isAlive(row))
            return table->card(row);
    }
    return std::nullopt;
}

void HeatSurfaceDefaultsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-heat-surface-defaults",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
