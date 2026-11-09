#pragma once

#include "oor/model/cards/dynamics/frfExcitationDofList/FrfExcitationDofListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct FrfExcitationDofListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfExcitationDofListCard& card);
    [[nodiscard]] static std::optional<FrfExcitationDofListCard> read(FieldReader& reader);
    static void write(const FrfExcitationDofListCard& card, FieldWriter& writer);
};

// All the FRFXIT1 cards. A card has no ID (IdSpace::None, category Other). The points of every card are one column of
// (first, last) runs: a single point is a run of one, a THRU range keeps its two ends.
class FrfExcitationDofListTable final : public TypedTable<FrfExcitationDofListTable, FrfExcitationDofListCard>
{
public:
    static constexpr std::string_view NAME = "FrfExcitationDofList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = FrfExcitationDofListFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfExcitationDofListCard&) noexcept { return 0; }
    std::uint32_t append(const FrfExcitationDofListCard& card, base::CardKey key);
    [[nodiscard]] FrfExcitationDofListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FrfExcitationDofListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& componentColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<std::int32_t> m_components;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
