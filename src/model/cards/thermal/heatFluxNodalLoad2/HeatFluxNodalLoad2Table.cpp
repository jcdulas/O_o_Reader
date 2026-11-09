#include "oor/model/cards/thermal/heatFluxNodalLoad2/HeatFluxNodalLoad2Table.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <string>

namespace oor::model
{

std::optional<HeatFluxNodalLoad2Card> HeatFluxNodalLoad2Format::read(FieldReader& reader)
{
    HeatFluxNodalLoad2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    for (std::size_t index = 0; index < HeatFluxNodalLoad2Card::POINT_COUNT; ++index)
    {
        const std::string name = "heatFlux" + std::to_string(index + 1);
        card.heatFlux[index] = reader.real(3 + static_cast<int>(index), name.c_str(), 0.0);
    }
    requireUnusedBlank(reader, {}, 10);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HeatFluxNodalLoad2Format::check(const HeatFluxNodalLoad2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.elemId <= 0)
        return CardProblem{"card.bad-id", "the surface element ID must be a positive integer", "elemId"};
    return std::nullopt;
}

void HeatFluxNodalLoad2Format::write(const HeatFluxNodalLoad2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    for (std::size_t index = 0; index < HeatFluxNodalLoad2Card::POINT_COUNT; ++index)
        writer.realUnlessDefault(3 + static_cast<int>(index), card.heatFlux[index], 0.0);
}

std::uint32_t HeatFluxNodalLoad2Table::append(const HeatFluxNodalLoad2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_elemId.push(card.elemId);
    m_heatFlux.push(card.heatFlux);
    return row;
}

HeatFluxNodalLoad2Card HeatFluxNodalLoad2Table::card(std::uint32_t row) const
{
    HeatFluxNodalLoad2Card card;
    card.id = m_id.get(row);
    card.elemId = m_elemId.get(row);
    card.heatFlux = m_heatFlux.get(row);
    return card;
}

void HeatFluxNodalLoad2Table::store(std::uint32_t row, const HeatFluxNodalLoad2Card& card)
{
    m_id.set(row, card.id);
    m_elemId.set(row, card.elemId);
    m_heatFlux.set(row, card.heatFlux);
}

void HeatFluxNodalLoad2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_elemId.keepRows(rows);
    m_heatFlux.keepRows(rows);
}

void HeatFluxNodalLoad2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_elemId.set(row, visitor.visit("elemId", ReferenceTarget::SurfaceElement, m_elemId.get(row)));
}

} // namespace oor::model
