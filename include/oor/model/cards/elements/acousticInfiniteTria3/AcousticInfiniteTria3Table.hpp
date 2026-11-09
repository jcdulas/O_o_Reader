#pragma once

#include "oor/model/cards/elements/acousticInfiniteTria3/AcousticInfiniteTria3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticInfiniteTria3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticInfiniteTria3Card& card);
    [[nodiscard]] static std::optional<AcousticInfiniteTria3Card> read(FieldReader& reader);
    static void write(const AcousticInfiniteTria3Card& card, FieldWriter& writer);
};

// All the CACINF3 elements: an element ID, in the space every element shares (IdSpace::Element); PID names a PACINF
// and the three points are grid points.
class AcousticInfiniteTria3Table final : public TypedTable<AcousticInfiniteTria3Table, AcousticInfiniteTria3Card>
{
public:
    static constexpr std::string_view NAME = "AcousticInfiniteTria3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AcousticInfiniteTria3Format;

    [[nodiscard]] static base::EntityId cardId(const AcousticInfiniteTria3Card& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticInfiniteTria3Card& card, base::CardKey key);
    [[nodiscard]] AcousticInfiniteTria3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticInfiniteTria3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, AcousticInfiniteTria3Card::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, AcousticInfiniteTria3Card::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
