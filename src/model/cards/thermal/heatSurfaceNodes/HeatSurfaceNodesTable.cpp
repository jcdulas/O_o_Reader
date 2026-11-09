#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/cards/thermal/heatSurfaceDefaults/HeatSurfaceDefaultsTable.hpp"
#include "oor/model/core/Model.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr std::string_view TYPES[] = {"ReversedNormal", "ReversedNormal1", "AreaTria3",
                                      "AreaQuad4",      "AreaTria6",       "AreaQuad8"};

} // namespace

bool HeatSurfaceNodesFormat::isSurfaceType(std::string_view type) noexcept
{
    return std::find(std::begin(TYPES), std::end(TYPES), type) != std::end(TYPES);
}

std::size_t HeatSurfaceNodesFormat::requiredNodeCount(std::string_view type) noexcept
{
    if (type == "ReversedNormal" || type == "ReversedNormal1")
        return 2;
    if (type == "AreaTria3" || type == "AreaTria6")
        return 3;
    if (type == "AreaQuad4" || type == "AreaQuad8")
        return 4;
    return 0;
}

std::size_t HeatSurfaceNodesFormat::maximumNodeCount(std::string_view type) noexcept
{
    if (type == "ReversedNormal" || type == "ReversedNormal1" || type == "AreaTria3")
        return 3;
    if (type == "AreaQuad4")
        return 4;
    if (type == "AreaTria6")
        return 6;
    if (type == "AreaQuad8")
        return 8;
    return 0;
}

std::optional<HeatSurfaceNodesCard> HeatSurfaceNodesFormat::read(FieldReader& reader)
{
    HeatSurfaceNodesCard card;
    card.id = reader.requiredInteger(1, "id");
    // A blank TYPE takes the TYPE of the BDYOR, given in finishRead once the whole
    // Bulk Data is read.
    card.kind =
        readKeyword(reader, 3, "kind",
                    {"ReversedNormal", "ReversedNormal1", "AreaTria3", "AreaQuad4", "AreaTria6", "AreaQuad8"}, "");
    card.frontViewId = reader.integer(4, "frontViewId", 0);
    card.backViewId = reader.integer(5, "backViewId", 0);
    card.frontRadiationPropId = reader.integer(6, "frontRadiationPropId", 0);
    card.backRadiationPropId = reader.integer(7, "backRadiationPropId", 0);
    for (std::size_t index = 0; index < HeatSurfaceNodesCard::NODE_COUNT; ++index)
        card.nodes[index] = reader.integer(9 + static_cast<int>(index), "node", 0);
    requireUnusedBlank(reader, {2, 8}, 16);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatSurfaceNodesFormat::check(const HeatSurfaceNodesCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (!card.kind.empty() && !isSurfaceType(card.kind))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be REV, REV1, AREA3, AREA4, AREA6, AREA8 or blank",
                           "kind"};
    if (card.frontViewId < 0 || card.backViewId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "frontViewId"};
    if (card.frontRadiationPropId < 0 || card.backRadiationPropId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative",
                           "frontRadiationPropId"};
    // A blank TYPE (from the BDYOR) is checked with the type once it has one.
    const std::size_t required = card.kind.empty() ? 0 : requiredNodeCount(card.kind);
    const std::size_t maximum = card.kind.empty() ? HeatSurfaceNodesCard::NODE_COUNT : maximumNodeCount(card.kind);
    for (std::size_t index = 0; index < HeatSurfaceNodesCard::NODE_COUNT; ++index)
    {
        const base::EntityId node = card.nodes[index];
        if (node < 0)
            return CardProblem{"card.bad-id", "a grid point ID cannot be negative", "node"};
        if (index < required && node == 0)
            return CardProblem{
                "card.missing-field",
                "a " + card.kind + " surface needs its first " + std::to_string(required) + " grid points", "node"};
        if (index >= maximum && node != 0)
            return CardProblem{"card.bad-value",
                               "a " + card.kind + " surface has at most " + std::to_string(maximum) + " grid points",
                               "node"};
    }
    // A grid point is named once.
    for (std::size_t first = 0; first < HeatSurfaceNodesCard::NODE_COUNT; ++first)
    {
        for (std::size_t second = first + 1; second < HeatSurfaceNodesCard::NODE_COUNT; ++second)
        {
            if (card.nodes[first] != 0 && card.nodes[first] == card.nodes[second])
                return CardProblem{"card.bad-value", "a grid point is repeated in the surface", "node"};
        }
    }
    return std::nullopt;
}

void HeatSurfaceNodesFormat::write(const HeatSurfaceNodesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (!card.kind.empty())
        writer.text(3, card.kind);
    writer.integerUnlessDefault(4, card.frontViewId, 0);
    writer.integerUnlessDefault(5, card.backViewId, 0);
    writer.integerUnlessDefault(6, card.frontRadiationPropId, 0);
    writer.integerUnlessDefault(7, card.backRadiationPropId, 0);
    for (std::size_t index = 0; index < HeatSurfaceNodesCard::NODE_COUNT; ++index)
        writer.integerUnlessDefault(9 + static_cast<int>(index), card.nodes[index], 0);
}

std::uint32_t HeatSurfaceNodesTable::append(const HeatSurfaceNodesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_kind.push(card.kind);
    m_frontViewId.push(card.frontViewId);
    m_backViewId.push(card.backViewId);
    m_frontRadiationPropId.push(card.frontRadiationPropId);
    m_backRadiationPropId.push(card.backRadiationPropId);
    m_nodes.push(card.nodes);
    return row;
}

HeatSurfaceNodesCard HeatSurfaceNodesTable::card(std::uint32_t row) const
{
    HeatSurfaceNodesCard card;
    card.id = m_id.get(row);
    card.kind = m_kind.get(row);
    card.frontViewId = m_frontViewId.get(row);
    card.backViewId = m_backViewId.get(row);
    card.frontRadiationPropId = m_frontRadiationPropId.get(row);
    card.backRadiationPropId = m_backRadiationPropId.get(row);
    card.nodes = m_nodes.get(row);
    return card;
}

void HeatSurfaceNodesTable::store(std::uint32_t row, const HeatSurfaceNodesCard& card)
{
    m_id.set(row, card.id);
    m_kind.set(row, card.kind);
    m_frontViewId.set(row, card.frontViewId);
    m_backViewId.set(row, card.backViewId);
    m_frontRadiationPropId.set(row, card.frontRadiationPropId);
    m_backRadiationPropId.set(row, card.backRadiationPropId);
    m_nodes.set(row, card.nodes);
}

void HeatSurfaceNodesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_kind.keepRows(rows);
    m_frontViewId.keepRows(rows);
    m_backViewId.keepRows(rows);
    m_frontRadiationPropId.keepRows(rows);
    m_backRadiationPropId.keepRows(rows);
    m_nodes.keepRows(rows);
}

void HeatSurfaceNodesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
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
    std::array<base::EntityId, HeatSurfaceNodesCard::NODE_COUNT> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

void HeatSurfaceNodesTable::finishRead(Model& model, const ReadContext& context)
{
    const std::optional<HeatSurfaceDefaultsCard> defaults = heatSurfaceDefaultsDefaults(model);
    // The surface type a BDYOR gives a CHBDYG with a blank TYPE: one of the types of a CHBDYG (REV, AREA3, AREA4,
    // AREA6, AREA8), or nothing.
    std::string defaultType;
    if (defaults && defaults->kind)
    {
        const std::string_view word = HEAT_SURFACE_TYPE_WORDS[static_cast<std::size_t>(*defaults->kind)];
        if (HeatSurfaceNodesFormat::isSurfaceType(word))
            defaultType = std::string(word);
    }
    // A diagnostic shows the deck: the card and its fields by their keywords.
    const std::string label = std::string(NAME);
    const std::string defaultsLabel = std::string(HeatSurfaceDefaultsTable::NAME);
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row) || !m_kind.get(row).empty())
            continue;
        if (defaultType.empty())
        {
            context.sink.error("card.missing-field",
                               label + " " + std::to_string(m_id.get(row)) + ": " + std::string("kind") +
                                   " is blank and no " + defaultsLabel + " gives the surface type of a " + label,
                               base::SourceLocation{"", 0, label, std::string("kind")});
            continue;
        }
        HeatSurfaceNodesCard filled = card(row);
        filled.kind = defaultType;
        if (const std::optional<CardProblem> problem = HeatSurfaceNodesFormat::check(filled))
        {
            const auto labelOf = [&](std::string_view field) { return std::string(field); };
            context.sink.error(problem->code,
                               label + " " + std::to_string(filled.id) + " with the " + labelOf("kind") + " " +
                                   defaultType + " of the " + defaultsLabel + ": " + problem->text(),
                               base::SourceLocation{"", 0, label, std::string(problem->field)});
            continue;
        }
        m_kind.set(row, defaultType);
    }
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
