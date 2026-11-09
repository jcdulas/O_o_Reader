#pragma once

#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SinglePointConstrUnionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SinglePointConstrUnionCard& card);
    [[nodiscard]] static std::optional<SinglePointConstrUnionCard> read(FieldReader& reader);
    static void write(const SinglePointConstrUnionCard& card, FieldWriter& writer);
};

// All the SPCADD cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The sets of every card are
// one list in CSR form.
class SinglePointConstrUnionTable final : public TypedTable<SinglePointConstrUnionTable, SinglePointConstrUnionCard>
{
public:
    static constexpr std::string_view NAME = "SinglePointConstrUnion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = SinglePointConstrUnionFormat;

    [[nodiscard]] static base::EntityId cardId(const SinglePointConstrUnionCard& card) noexcept { return card.id; }
    std::uint32_t append(const SinglePointConstrUnionCard& card, base::CardKey key);
    [[nodiscard]] SinglePointConstrUnionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SinglePointConstrUnionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& setsColumn() noexcept { return m_sets; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_sets;
};

} // namespace oor::model
