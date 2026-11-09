#pragma once

#include "oor/model/cards/superelements/freeBoundaryDofs1/FreeBoundaryDofs1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct FreeBoundaryDofs1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeBoundaryDofs1Card& card);
    [[nodiscard]] static std::optional<FreeBoundaryDofs1Card> read(FieldReader& reader);
    static void write(const FreeBoundaryDofs1Card& card, FieldWriter& writer);
};

// All the BNDFRE1 cards. A card has no ID (IdSpace::None, category Set, like the CSET1 it equals). The points of every
// card are one column of (first, last) runs.
class FreeBoundaryDofs1Table final : public TypedTable<FreeBoundaryDofs1Table, FreeBoundaryDofs1Card>
{
public:
    static constexpr std::string_view NAME = "FreeBoundaryDofs1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = FreeBoundaryDofs1Format;

    [[nodiscard]] static base::EntityId cardId(const FreeBoundaryDofs1Card&) noexcept { return 0; }
    std::uint32_t append(const FreeBoundaryDofs1Card& card, base::CardKey key);
    [[nodiscard]] FreeBoundaryDofs1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeBoundaryDofs1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }
    [[nodiscard]] Column<std::uint8_t>& allColumn() noexcept { return m_all; }

private:
    Column<std::int32_t> m_components;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
    Column<std::uint8_t> m_all;
};

} // namespace oor::model
