#pragma once

#include "oor/model/cards/superelements/fixedBoundaryDofs1/FixedBoundaryDofs1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct FixedBoundaryDofs1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FixedBoundaryDofs1Card& card);
    [[nodiscard]] static std::optional<FixedBoundaryDofs1Card> read(FieldReader& reader);
    static void write(const FixedBoundaryDofs1Card& card, FieldWriter& writer);
};

// All the BNDFIX1 cards. A card has no ID (IdSpace::None, category Set, like the BSET1 it equals). The points of every
// card are one column of (first, last) runs.
class FixedBoundaryDofs1Table final : public TypedTable<FixedBoundaryDofs1Table, FixedBoundaryDofs1Card>
{
public:
    static constexpr std::string_view NAME = "FixedBoundaryDofs1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = FixedBoundaryDofs1Format;

    [[nodiscard]] static base::EntityId cardId(const FixedBoundaryDofs1Card&) noexcept { return 0; }
    std::uint32_t append(const FixedBoundaryDofs1Card& card, base::CardKey key);
    [[nodiscard]] FixedBoundaryDofs1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FixedBoundaryDofs1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<std::int32_t> m_components;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
