#pragma once

#include "oor/model/cards/elements/springOffsetExplicit/SpringOffsetExplicitCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SpringOffsetExplicitFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SpringOffsetExplicitCard& card);
    [[nodiscard]] static std::optional<SpringOffsetExplicitCard> read(FieldReader& reader);
    static void write(const SpringOffsetExplicitCard& card, FieldWriter& writer);
};

// All the CSPR elements: an element in the space every element shares (IdSpace::Element). The nodes are one column of
// two IDs (an (N, 2) NumPy view); PID names a PSPRMAT.
class SpringOffsetExplicitTable final
    : public TypedTable<SpringOffsetExplicitTable, SpringOffsetExplicitCard>
{
public:
    static constexpr std::string_view NAME = "SpringOffsetExplicit";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = SpringOffsetExplicitFormat;

    [[nodiscard]] static base::EntityId cardId(const SpringOffsetExplicitCard& card) noexcept { return card.id; }
    std::uint32_t append(const SpringOffsetExplicitCard& card, base::CardKey key);
    [[nodiscard]] SpringOffsetExplicitCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SpringOffsetExplicitCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& nodesColumn() noexcept { return m_nodes; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, 2>> m_nodes;
};

} // namespace oor::model
