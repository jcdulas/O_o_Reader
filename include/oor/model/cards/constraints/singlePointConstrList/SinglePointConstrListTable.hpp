#pragma once

#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SinglePointConstrListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SinglePointConstrListCard& card);
    [[nodiscard]] static std::optional<SinglePointConstrListCard> read(FieldReader& reader);
    static void write(const SinglePointConstrListCard& card, FieldWriter& writer);
};

// All the SPC1 cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The node lists are one CSR
// column of (first, last) runs.
class SinglePointConstrListTable final : public TypedTable<SinglePointConstrListTable, SinglePointConstrListCard>
{
public:
    static constexpr std::string_view NAME = "SinglePointConstrList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = SinglePointConstrListFormat;

    [[nodiscard]] static base::EntityId cardId(const SinglePointConstrListCard& card) noexcept { return card.id; }
    std::uint32_t append(const SinglePointConstrListCard& card, base::CardKey key);
    [[nodiscard]] SinglePointConstrListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SinglePointConstrListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_components;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
