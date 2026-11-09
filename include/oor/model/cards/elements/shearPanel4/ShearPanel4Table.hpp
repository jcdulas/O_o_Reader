#pragma once

#include "oor/model/cards/elements/shearPanel4/ShearPanel4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShearPanel4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShearPanel4Card& card);
    [[nodiscard]] static std::optional<ShearPanel4Card> read(FieldReader& reader);
    static void write(const ShearPanel4Card& card, FieldWriter& writer);
};

// All the CSHEAR elements: the nodes are one column of four IDs per element (an (N, 4) NumPy view without copy).
class ShearPanel4Table final : public TypedTable<ShearPanel4Table, ShearPanel4Card>
{
public:
    static constexpr std::string_view NAME = "ShearPanel4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = ShearPanel4Format;

    [[nodiscard]] static base::EntityId cardId(const ShearPanel4Card& card) noexcept { return card.id; }
    std::uint32_t append(const ShearPanel4Card& card, base::CardKey key);
    [[nodiscard]] ShearPanel4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShearPanel4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& nodesColumn() noexcept { return m_nodes; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 4>> m_nodes;
};

} // namespace oor::model
