#include "oor/model/cards/thermal/heatFluxNodeSetLoad/HeatFluxNodeSetLoadTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

constexpr std::string_view FLAGS[] = {"Point",     "Line",      "ReversedNormal", "AreaTria3",
                                      "AreaQuad4", "AreaTria6", "AreaQuad8"};

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

bool HeatFluxNodeSetLoadFormat::isFlag(std::string_view flag) noexcept
{
    return std::find(std::begin(FLAGS), std::end(FLAGS), flag) != std::end(FLAGS);
}

std::size_t HeatFluxNodeSetLoadFormat::requiredNodeCount(std::string_view flag) noexcept
{
    if (flag == "Point")
        return 1;
    if (flag == "Line" || flag == "ReversedNormal")
        return 2;
    if (flag == "AreaTria3")
        return 3;
    if (flag == "AreaQuad4" || flag == "AreaTria6")
        return 4;
    if (flag == "AreaQuad8")
        return 5;
    return 0;
}

std::size_t HeatFluxNodeSetLoadFormat::maximumNodeCount(std::string_view flag) noexcept
{
    if (flag == "AreaTria6")
        return 6;
    if (flag == "AreaQuad8")
        return 8;
    return requiredNodeCount(flag);
}

HeatFluxNodeSetLoadTable::HeatFluxNodeSetLoadTable() : m_area(notGiven()) {}

std::optional<HeatFluxNodeSetLoadCard> HeatFluxNodeSetLoadFormat::read(FieldReader& reader)
{
    HeatFluxNodeSetLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.faceKind =
        readKeyword(reader, 2, "faceKind",
                    {"Point", "Line", "ReversedNormal", "AreaTria3", "AreaQuad4", "AreaTria6", "AreaQuad8"}, "");
    if (card.faceKind.empty() && !reader.failed())
        reader.error("card.missing-field", std::string("faceKind") + " (the type of face) is required", "faceKind");
    card.heatFlux = reader.requiredReal(3, "heatFlux");
    card.area = reader.optionalReal(4, "area");
    for (std::size_t index = 0; index < HeatFluxNodeSetLoadCard::NODE_COUNT; ++index)
        card.nodes[index] = reader.integer(5 + static_cast<int>(index), "node", 0);
    requireUnusedBlank(reader, {}, 12);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatFluxNodeSetLoadFormat::check(const HeatFluxNodeSetLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (!isFlag(card.faceKind))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be POINT, LINE, REV, AREA3, AREA4, AREA6 or AREA8",
                           "faceKind"};
    if (card.area && !(*card.area > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0 (or blank)",
                           "area"};
    const std::size_t required = requiredNodeCount(card.faceKind);
    const std::size_t maximum = maximumNodeCount(card.faceKind);
    for (std::size_t index = 0; index < HeatFluxNodeSetLoadCard::NODE_COUNT; ++index)
    {
        const base::EntityId node = card.nodes[index];
        if (node < 0)
            return CardProblem{"card.bad-id", "a grid point ID cannot be negative", "node"};
        if (index < required && node == 0)
            return CardProblem{
                "card.missing-field",
                "a " + card.faceKind + " face needs its first " + std::to_string(required) + " grid points", "node"};
        if (index >= maximum && node != 0)
            return CardProblem{"card.bad-value",
                               "a " + card.faceKind + " face has at most " + std::to_string(maximum) + " grid points",
                               "node"};
    }
    return std::nullopt;
}

void HeatFluxNodeSetLoadFormat::write(const HeatFluxNodeSetLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.faceKind);
    writer.real(3, card.heatFlux);
    if (card.area)
        writer.real(4, *card.area);
    for (std::size_t index = 0; index < HeatFluxNodeSetLoadCard::NODE_COUNT; ++index)
        writer.integerUnlessDefault(5 + static_cast<int>(index), card.nodes[index], 0);
}

std::uint32_t HeatFluxNodeSetLoadTable::append(const HeatFluxNodeSetLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_faceKind.push(card.faceKind);
    m_heatFlux.push(card.heatFlux);
    m_area.push(card.area.value_or(notGiven()));
    m_nodes.push(card.nodes);
    return row;
}

HeatFluxNodeSetLoadCard HeatFluxNodeSetLoadTable::card(std::uint32_t row) const
{
    HeatFluxNodeSetLoadCard card;
    card.id = m_id.get(row);
    card.faceKind = m_faceKind.get(row);
    card.heatFlux = m_heatFlux.get(row);
    if (const double area = m_area.get(row); !std::isnan(area))
        card.area = area;
    card.nodes = m_nodes.get(row);
    return card;
}

void HeatFluxNodeSetLoadTable::store(std::uint32_t row, const HeatFluxNodeSetLoadCard& card)
{
    m_id.set(row, card.id);
    m_faceKind.set(row, card.faceKind);
    m_heatFlux.set(row, card.heatFlux);
    m_area.set(row, card.area.value_or(notGiven()));
    m_nodes.set(row, card.nodes);
}

void HeatFluxNodeSetLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_faceKind.keepRows(rows);
    m_heatFlux.keepRows(rows);
    m_area.keepRows(rows);
    m_nodes.keepRows(rows);
}

void HeatFluxNodeSetLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, HeatFluxNodeSetLoadCard::NODE_COUNT> nodes = m_nodes.get(row);
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

} // namespace oor::model
