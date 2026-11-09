#pragma once

#include "oor/model/cards/elements/acousticInfiniteQuad4/AcousticInfiniteQuad4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticInfiniteQuad4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticInfiniteQuad4Card& card);
    [[nodiscard]] static std::optional<AcousticInfiniteQuad4Card> read(FieldReader& reader);
    static void write(const AcousticInfiniteQuad4Card& card, FieldWriter& writer);
};

// All the CACINF4 elements: an element ID, in the space every element shares (IdSpace::Element); PID names a PACINF
// and the four points are grid points.
class AcousticInfiniteQuad4Table final : public TypedTable<AcousticInfiniteQuad4Table, AcousticInfiniteQuad4Card>
{
public:
    static constexpr std::string_view NAME = "AcousticInfiniteQuad4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AcousticInfiniteQuad4Format;

    [[nodiscard]] static base::EntityId cardId(const AcousticInfiniteQuad4Card& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticInfiniteQuad4Card& card, base::CardKey key);
    [[nodiscard]] AcousticInfiniteQuad4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticInfiniteQuad4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, AcousticInfiniteQuad4Card::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, AcousticInfiniteQuad4Card::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
