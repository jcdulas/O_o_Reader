#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsTable.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

constexpr std::string_view TYPES[] = {"Point", "Line", "EllipticCylinder", "ForcedConvectionTube", "Tube"};

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

bool HeatSurfacePropertyFormat::isSurfaceType(std::string_view type) noexcept
{
    return std::find(std::begin(TYPES), std::end(TYPES), type) != std::end(TYPES);
}

HeatSurfacePropertyTable::HeatSurfacePropertyTable()
    : m_direction1(notGiven()), m_direction2(notGiven()), m_direction3(notGiven())
{
}

std::optional<HeatSurfacePropertyCard> HeatSurfacePropertyFormat::read(FieldReader& reader)
{
    HeatSurfacePropertyCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.kind =
        readKeyword(reader, 3, "kind", {"Point", "Line", "EllipticCylinder", "ForcedConvectionTube", "Tube"}, "");
    if (card.kind.empty() && !reader.failed())
        reader.error("card.missing-field", std::string("the surface ") + std::string("kind") + " is required", "kind");
    card.frontViewId = reader.integer(4, "frontViewId", 0);
    card.backViewId = reader.integer(5, "backViewId", 0);
    card.node1 = reader.requiredInteger(6, "node1");
    card.node2 = reader.integer(7, "node2", 0);
    card.orientationNode = reader.integer(8, "orientationNode", 0);
    card.frontRadiationPropId = reader.integer(9, "frontRadiationPropId", 0);
    card.backRadiationPropId = reader.integer(10, "backRadiationPropId", 0);
    card.midsideNode = reader.integer(11, "midsideNode", 0);
    card.directionFrameId = reader.integer(12, "directionFrameId", 0);
    card.direction1 = reader.optionalReal(13, "direction1");
    card.direction2 = reader.optionalReal(14, "direction2");
    card.direction3 = reader.optionalReal(15, "direction3");
    requireUnusedBlank(reader, {}, 15);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatSurfacePropertyFormat::check(const HeatSurfacePropertyCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the surface property ID must be a positive integer", "propId"};
    if (!isSurfaceType(card.kind))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be POINT, LINE, ELCYL, FTUBE or TUBE", "kind"};
    if (card.frontViewId < 0 || card.backViewId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "frontViewId"};
    if (card.frontRadiationPropId < 0 || card.backRadiationPropId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative",
                           "frontRadiationPropId"};
    if (card.node1 <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive grid point ID",
                           "node1"};
    if (card.kind == "Point")
    {
        // A POINT has one primary grid point.
        if (card.node2 != 0)
            return CardProblem{"card.bad-value",
                               std::string("a POINT surface has ") + CardProblem::fieldNamed("node1") +
                                   " only: " + std::string(CardProblem::FIELD) + " must be blank",
                               "node2"};
    }
    else
    {
        if (card.node2 <= 0)
            return CardProblem{"card.missing-field",
                               "a " + card.kind + " surface needs " + std::string(CardProblem::FIELD), "node2"};
        if (card.node2 == card.node1)
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("node1") + " and " + std::string(CardProblem::FIELD) +
                                   " must be different grid points",
                               "node2"};
    }
    if (card.orientationNode < 0 || card.midsideNode < 0)
        return CardProblem{"card.bad-id", "a grid point ID cannot be negative", "orientationNode"};
    if (card.directionFrameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "directionFrameId"};
    return std::nullopt;
}

void HeatSurfacePropertyFormat::write(const HeatSurfacePropertyCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.text(3, card.kind);
    writer.integerUnlessDefault(4, card.frontViewId, 0);
    writer.integerUnlessDefault(5, card.backViewId, 0);
    writer.integer(6, card.node1);
    writer.integerUnlessDefault(7, card.node2, 0);
    writer.integerUnlessDefault(8, card.orientationNode, 0);
    writer.integerUnlessDefault(9, card.frontRadiationPropId, 0);
    writer.integerUnlessDefault(10, card.backRadiationPropId, 0);
    writer.integerUnlessDefault(11, card.midsideNode, 0);
    writer.integerUnlessDefault(12, card.directionFrameId, 0);
    if (card.direction1)
        writer.real(13, *card.direction1);
    if (card.direction2)
        writer.real(14, *card.direction2);
    if (card.direction3)
        writer.real(15, *card.direction3);
}

std::uint32_t HeatSurfacePropertyTable::append(const HeatSurfacePropertyCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_kind.push(card.kind);
    m_frontViewId.push(card.frontViewId);
    m_backViewId.push(card.backViewId);
    m_node1.push(card.node1);
    m_node2.push(card.node2);
    m_orientationNode.push(card.orientationNode);
    m_frontRadiationPropId.push(card.frontRadiationPropId);
    m_backRadiationPropId.push(card.backRadiationPropId);
    m_midsideNode.push(card.midsideNode);
    m_directionFrameId.push(card.directionFrameId);
    m_direction1.push(card.direction1.value_or(notGiven()));
    m_direction2.push(card.direction2.value_or(notGiven()));
    m_direction3.push(card.direction3.value_or(notGiven()));
    return row;
}

HeatSurfacePropertyCard HeatSurfacePropertyTable::card(std::uint32_t row) const
{
    HeatSurfacePropertyCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.kind = m_kind.get(row);
    card.frontViewId = m_frontViewId.get(row);
    card.backViewId = m_backViewId.get(row);
    card.node1 = m_node1.get(row);
    card.node2 = m_node2.get(row);
    card.orientationNode = m_orientationNode.get(row);
    card.frontRadiationPropId = m_frontRadiationPropId.get(row);
    card.backRadiationPropId = m_backRadiationPropId.get(row);
    card.midsideNode = m_midsideNode.get(row);
    card.directionFrameId = m_directionFrameId.get(row);
    card.direction1 = given(m_direction1.get(row));
    card.direction2 = given(m_direction2.get(row));
    card.direction3 = given(m_direction3.get(row));
    return card;
}

void HeatSurfacePropertyTable::store(std::uint32_t row, const HeatSurfacePropertyCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_kind.set(row, card.kind);
    m_frontViewId.set(row, card.frontViewId);
    m_backViewId.set(row, card.backViewId);
    m_node1.set(row, card.node1);
    m_node2.set(row, card.node2);
    m_orientationNode.set(row, card.orientationNode);
    m_frontRadiationPropId.set(row, card.frontRadiationPropId);
    m_backRadiationPropId.set(row, card.backRadiationPropId);
    m_midsideNode.set(row, card.midsideNode);
    m_directionFrameId.set(row, card.directionFrameId);
    m_direction1.set(row, card.direction1.value_or(notGiven()));
    m_direction2.set(row, card.direction2.value_or(notGiven()));
    m_direction3.set(row, card.direction3.value_or(notGiven()));
}

void HeatSurfacePropertyTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_kind.keepRows(rows);
    m_frontViewId.keepRows(rows);
    m_backViewId.keepRows(rows);
    m_node1.keepRows(rows);
    m_node2.keepRows(rows);
    m_orientationNode.keepRows(rows);
    m_frontRadiationPropId.keepRows(rows);
    m_backRadiationPropId.keepRows(rows);
    m_midsideNode.keepRows(rows);
    m_directionFrameId.keepRows(rows);
    m_direction1.keepRows(rows);
    m_direction2.keepRows(rows);
    m_direction3.keepRows(rows);
}

void HeatSurfacePropertyTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_propId.set(row, visitor.visit("propId", ReferenceTarget::HeatBoundaryProperty, m_propId.get(row)));
    if (const base::EntityId frontViewId = m_frontViewId.get(row); frontViewId > 0)
        m_frontViewId.set(row, visitor.visit("frontViewId", ReferenceTarget::ViewFactorFaceFactor, frontViewId));
    if (const base::EntityId backViewId = m_backViewId.get(row); backViewId > 0)
        m_backViewId.set(row, visitor.visit("backViewId", ReferenceTarget::ViewFactorFaceFactor, backViewId));
    m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, m_node1.get(row)));
    if (const base::EntityId node2 = m_node2.get(row); node2 > 0)
        m_node2.set(row, visitor.visit("node2", ReferenceTarget::Node, node2));
    if (const base::EntityId orientationNode = m_orientationNode.get(row); orientationNode > 0)
        m_orientationNode.set(row, visitor.visit("orientationNode", ReferenceTarget::Node, orientationNode));
    if (const base::EntityId frontRadiationPropId = m_frontRadiationPropId.get(row); frontRadiationPropId > 0)
        m_frontRadiationPropId.set(
            row, visitor.visit("frontRadiationPropId", ReferenceTarget::RadiationMaterial, frontRadiationPropId));
    if (const base::EntityId backRadiationPropId = m_backRadiationPropId.get(row); backRadiationPropId > 0)
        m_backRadiationPropId.set(
            row, visitor.visit("backRadiationPropId", ReferenceTarget::RadiationMaterial, backRadiationPropId));
    if (const base::EntityId midsideNode = m_midsideNode.get(row); midsideNode > 0)
        m_midsideNode.set(row, visitor.visit("midsideNode", ReferenceTarget::Node, midsideNode));
    if (const base::EntityId directionFrameId = m_directionFrameId.get(row); directionFrameId > 0)
        m_directionFrameId.set(row, visitor.visit("directionFrameId", ReferenceTarget::Coordinate, directionFrameId));
}

void HeatSurfacePropertyTable::finishRead(Model& model, const ReadContext&)
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
        // The orientation: a card that names a grid point or a vector of its own keeps it; else the grid point of the
        // BDYOR if it names one, else its vector (the vector is not used when a grid point is). The system CE of a
        // vector that is in use defaults to the one of the BDYOR.
        const bool ownVector = !std::isnan(m_direction1.get(row)) || !std::isnan(m_direction2.get(row)) ||
                               !std::isnan(m_direction3.get(row));
        if (m_orientationNode.get(row) == 0 && !ownVector)
        {
            if (defaults->orientationNode > 0)
            {
                m_orientationNode.set(row, defaults->orientationNode);
            }
            else if (defaults->direction1 || defaults->direction2 || defaults->direction3)
            {
                m_direction1.set(row, defaults->direction1.value_or(notGiven()));
                m_direction2.set(row, defaults->direction2.value_or(notGiven()));
                m_direction3.set(row, defaults->direction3.value_or(notGiven()));
            }
        }
        const bool usesVector = m_orientationNode.get(row) == 0 &&
                                (!std::isnan(m_direction1.get(row)) || !std::isnan(m_direction2.get(row)) ||
                                 !std::isnan(m_direction3.get(row)));
        if (usesVector && m_directionFrameId.get(row) == 0)
            m_directionFrameId.set(row, defaults->directionFrameId);
    }
}

} // namespace oor::model
