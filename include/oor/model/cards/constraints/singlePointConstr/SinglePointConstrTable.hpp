#pragma once

#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SinglePointConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SinglePointConstrCard& card);
    [[nodiscard]] static std::optional<SinglePointConstrCard> read(FieldReader& reader);
    static void write(const SinglePointConstrCard& card, FieldWriter& writer);
};

// All the SPC cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The two groups of a card are
// kept as three columns of two values.
class SinglePointConstrTable final : public TypedTable<SinglePointConstrTable, SinglePointConstrCard>
{
public:
    static constexpr std::string_view NAME = "SinglePointConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = SinglePointConstrFormat;

    [[nodiscard]] static base::EntityId cardId(const SinglePointConstrCard& card) noexcept { return card.id; }
    std::uint32_t append(const SinglePointConstrCard& card, base::CardKey key);
    [[nodiscard]] SinglePointConstrCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SinglePointConstrCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& nodesColumn() noexcept { return m_nodes; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::array<double, 2>>& enforcedColumn() noexcept { return m_enforced; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 2>> m_nodes;
    Column<std::array<std::int32_t, 2>> m_components;
    Column<std::array<double, 2>> m_enforced;
};

} // namespace oor::model
