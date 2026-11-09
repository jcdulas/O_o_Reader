#pragma once

#include "oor/model/cards/constraints/multiPointConstrUnion/MultiPointConstrUnionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MultiPointConstrUnionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MultiPointConstrUnionCard& card);
    [[nodiscard]] static std::optional<MultiPointConstrUnionCard> read(FieldReader& reader);
    static void write(const MultiPointConstrUnionCard& card, FieldWriter& writer);
};

// All the MPCADD cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The sets of every card are
// one list in CSR form.
class MultiPointConstrUnionTable final : public TypedTable<MultiPointConstrUnionTable, MultiPointConstrUnionCard>
{
public:
    static constexpr std::string_view NAME = "MultiPointConstrUnion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = MultiPointConstrUnionFormat;

    [[nodiscard]] static base::EntityId cardId(const MultiPointConstrUnionCard& card) noexcept { return card.id; }
    std::uint32_t append(const MultiPointConstrUnionCard& card, base::CardKey key);
    [[nodiscard]] MultiPointConstrUnionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MultiPointConstrUnionCard& card);
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
