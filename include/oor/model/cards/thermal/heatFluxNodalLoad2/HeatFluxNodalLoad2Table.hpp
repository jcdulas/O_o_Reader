#pragma once

#include "oor/model/cards/thermal/heatFluxNodalLoad2/HeatFluxNodalLoad2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct HeatFluxNodalLoad2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatFluxNodalLoad2Card& card);
    [[nodiscard]] static std::optional<HeatFluxNodalLoad2Card> read(FieldReader& reader);
    static void write(const HeatFluxNodalLoad2Card& card, FieldWriter& writer);
};

// All the QBDY2 cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load).
class HeatFluxNodalLoad2Table final : public TypedTable<HeatFluxNodalLoad2Table, HeatFluxNodalLoad2Card>
{
public:
    static constexpr std::string_view NAME = "HeatFluxNodalLoad2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatFluxNodalLoad2Format;

    [[nodiscard]] static base::EntityId cardId(const HeatFluxNodalLoad2Card& card) noexcept { return card.id; }
    std::uint32_t append(const HeatFluxNodalLoad2Card& card, base::CardKey key);
    [[nodiscard]] HeatFluxNodalLoad2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatFluxNodalLoad2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& elemIdColumn() noexcept { return m_elemId; }
    [[nodiscard]] Column<std::array<double, HeatFluxNodalLoad2Card::POINT_COUNT>>& heatFluxColumn() noexcept
    {
        return m_heatFlux;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_elemId;
    Column<std::array<double, HeatFluxNodalLoad2Card::POINT_COUNT>> m_heatFlux;
};

} // namespace oor::model
