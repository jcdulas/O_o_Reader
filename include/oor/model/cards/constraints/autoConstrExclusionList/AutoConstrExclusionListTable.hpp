#pragma once

#include "oor/model/cards/constraints/autoConstrExclusionList/AutoConstrExclusionListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AutoConstrExclusionListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AutoConstrExclusionListCard& card);
    [[nodiscard]] static std::optional<AutoConstrExclusionListCard> read(FieldReader& reader);
    static void write(const AutoConstrExclusionListCard& card, FieldWriter& writer);
};

// All the SPCOFF1 cards. A card has no ID (IdSpace::None). The points of every card are one column of (first, last)
// runs.
class AutoConstrExclusionListTable final : public TypedTable<AutoConstrExclusionListTable, AutoConstrExclusionListCard>
{
public:
    static constexpr std::string_view NAME = "AutoConstrExclusionList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::None};
    using Format = AutoConstrExclusionListFormat;

    [[nodiscard]] static base::EntityId cardId(const AutoConstrExclusionListCard&) noexcept { return 0; }
    std::uint32_t append(const AutoConstrExclusionListCard& card, base::CardKey key);
    [[nodiscard]] AutoConstrExclusionListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AutoConstrExclusionListCard& card);
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
