#pragma once

#include "oor/model/cards/constraints/relativeEnforcedMotionConstr/RelativeEnforcedMotionConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct RelativeEnforcedMotionConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RelativeEnforcedMotionConstrCard& card);
    [[nodiscard]] static std::optional<RelativeEnforcedMotionConstrCard> read(FieldReader& reader);
    static void write(const RelativeEnforcedMotionConstrCard& card, FieldWriter& writer);
};

// All the SPCR cards. The ID is a load set ID shared by many cards (IdSpace::Group), as for the SPCD: an SPCR is
// selected the way a load is, not by the SPC command. The two groups of a card are kept as three columns of two values.
// The points are references to grid or scalar points (a scalar point that no SPOINT defines is allowed).
class RelativeEnforcedMotionConstrTable final
    : public TypedTable<RelativeEnforcedMotionConstrTable, RelativeEnforcedMotionConstrCard>
{
public:
    static constexpr std::string_view NAME = "RelativeEnforcedMotionConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = RelativeEnforcedMotionConstrFormat;

    [[nodiscard]] static base::EntityId cardId(const RelativeEnforcedMotionConstrCard& card) noexcept
    {
        return card.id;
    }
    std::uint32_t append(const RelativeEnforcedMotionConstrCard& card, base::CardKey key);
    [[nodiscard]] RelativeEnforcedMotionConstrCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RelativeEnforcedMotionConstrCard& card);
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
