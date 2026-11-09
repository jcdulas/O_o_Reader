#pragma once

#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct FacePressureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FacePressureLoadCard& card);
    [[nodiscard]] static std::optional<FacePressureLoadCard> read(FieldReader& reader);
    static void write(const FacePressureLoadCard& card, FieldWriter& writer);
};

// All the PLOAD4 cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class FacePressureLoadTable final : public TypedTable<FacePressureLoadTable, FacePressureLoadCard>
{
public:
    static constexpr std::string_view NAME = "FacePressureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = FacePressureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const FacePressureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const FacePressureLoadCard& card, base::CardKey key);
    [[nodiscard]] FacePressureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FacePressureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& elemIdColumn() noexcept { return m_elemId; }
    [[nodiscard]] Column<base::EntityId>& elemId2Column() noexcept { return m_elemId2; }
    [[nodiscard]] Column<std::array<double, 4>>& pressuresColumn() noexcept { return m_pressures; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<base::EntityId>& node3Or4Column() noexcept { return m_node3Or4; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<std::array<double, 3>>& directionColumn() noexcept { return m_direction; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<std::uint8_t>& lineDirectionColumn() noexcept { return m_lineDirection; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_elemId;
    Column<base::EntityId> m_elemId2;
    Column<std::array<double, 4>> m_pressures;
    Column<base::EntityId> m_node1;
    Column<base::EntityId> m_node3Or4;
    Column<base::EntityId> m_frameId;
    Column<std::array<double, 3>> m_direction;
    Column<std::uint8_t> m_kind;
    Column<std::uint8_t> m_lineDirection;
};

} // namespace oor::model
