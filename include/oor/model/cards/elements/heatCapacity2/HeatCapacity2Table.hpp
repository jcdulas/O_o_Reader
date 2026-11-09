#pragma once

#include "oor/model/cards/elements/heatCapacity2/HeatCapacity2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatCapacity2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatCapacity2Card& card);
    [[nodiscard]] static std::optional<HeatCapacity2Card> read(FieldReader& reader);
    static void write(const HeatCapacity2Card& card, FieldWriter& writer);
};

// All the CDAMP5 elements.
class HeatCapacity2Table final : public TypedTable<HeatCapacity2Table, HeatCapacity2Card>
{
public:
    static constexpr std::string_view NAME = "HeatCapacity2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = HeatCapacity2Format;

    HeatCapacity2Table() : m_node1(0), m_node2(0) {}

    [[nodiscard]] static base::EntityId cardId(const HeatCapacity2Card& card) noexcept { return card.id; }
    std::uint32_t append(const HeatCapacity2Card& card, base::CardKey key);
    [[nodiscard]] HeatCapacity2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatCapacity2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_node1;
    Column<base::EntityId> m_node2;
};

} // namespace oor::model
