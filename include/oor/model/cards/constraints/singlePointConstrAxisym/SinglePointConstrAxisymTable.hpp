#pragma once

#include "oor/model/cards/constraints/singlePointConstrAxisym/SinglePointConstrAxisymCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct SinglePointConstrAxisymFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SinglePointConstrAxisymCard& card);
    [[nodiscard]] static std::optional<SinglePointConstrAxisymCard> read(FieldReader& reader);
    static void write(const SinglePointConstrAxisymCard& card, FieldWriter& writer);
};

// All the SPCAX cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). RID names a RINGAX point,
// which is not a card of the model yet: it is a ReferenceTarget::Other (not resolved, not renumbered).
class SinglePointConstrAxisymTable final : public TypedTable<SinglePointConstrAxisymTable, SinglePointConstrAxisymCard>
{
public:
    static constexpr std::string_view NAME = "SinglePointConstrAxisym";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = SinglePointConstrAxisymFormat;

    [[nodiscard]] static base::EntityId cardId(const SinglePointConstrAxisymCard& card) noexcept { return card.id; }
    std::uint32_t append(const SinglePointConstrAxisymCard& card, base::CardKey key);
    [[nodiscard]] SinglePointConstrAxisymCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SinglePointConstrAxisymCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] Column<std::int32_t>& harmonicColumn() noexcept { return m_harmonic; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<double>& enforcedColumn() noexcept { return m_enforced; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ringId;
    Column<std::int32_t> m_harmonic;
    Column<std::int32_t> m_components;
    Column<double> m_enforced;
};

} // namespace oor::model
