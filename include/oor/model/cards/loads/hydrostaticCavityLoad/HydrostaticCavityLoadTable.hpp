#pragma once

#include "oor/model/cards/loads/hydrostaticCavityLoad/HydrostaticCavityLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct HydrostaticCavityLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HydrostaticCavityLoadCard& card);
    [[nodiscard]] static std::optional<HydrostaticCavityLoadCard> read(FieldReader& reader);
    static void write(const HydrostaticCavityLoadCard& card, FieldWriter& writer);
};

// All the HYDROC cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). PSOLID is a
// Property reference; CID/GRD is a Coordinate reference when it is above zero and a Node reference (of its absolute
// value, the sign staying on the card) when it is below zero.
class HydrostaticCavityLoadTable final : public TypedTable<HydrostaticCavityLoadTable, HydrostaticCavityLoadCard>
{
public:
    static constexpr std::string_view NAME = "HydrostaticCavityLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HydrostaticCavityLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const HydrostaticCavityLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const HydrostaticCavityLoadCard& card, base::CardKey key);
    [[nodiscard]] HydrostaticCavityLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HydrostaticCavityLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<double>& nominalPressureColumn() noexcept { return m_nominalPressure; }
    [[nodiscard]] Column<double>& gravityColumn() noexcept { return m_gravity; }
    [[nodiscard]] Column<std::int32_t>& punchOptionColumn() noexcept { return m_punchOption; }
    [[nodiscard]] Column<std::int32_t>& punchPressureLoadsColumn() noexcept { return m_punchPressureLoads; }
    [[nodiscard]] Column<std::int32_t>& overrideOptionColumn() noexcept { return m_overrideOption; }
    [[nodiscard]] Column<base::EntityId>& frameOrNodeColumn() noexcept { return m_frameOrNode; }
    [[nodiscard]] Column<double>& baseHeightColumn() noexcept { return m_baseHeight; }
    [[nodiscard]] Column<double>& heightAboveColumn() noexcept { return m_heightAbove; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<double> m_nominalPressure;
    Column<double> m_gravity{1.0}; // the default of G
    Column<std::int32_t> m_punchOption;
    Column<std::int32_t> m_punchPressureLoads;
    Column<std::int32_t> m_overrideOption;
    Column<base::EntityId> m_frameOrNode;
    Column<double> m_baseHeight;
    Column<double> m_heightAbove;
};

} // namespace oor::model
