#pragma once

#include "oor/model/cards/nodes/scalarPoint/ScalarPointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ScalarPointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ScalarPointCard& card);
    // One SPOINT card gives one point per ID listed, a THRU range giving all the IDs of the range.
    [[nodiscard]] static std::vector<ScalarPointCard> readAll(FieldReader& reader);
    static void write(const ScalarPointCard& card, FieldWriter& writer);
};

// All the scalar points defined by SPOINT cards: one row per point, in the node ID space (so a scalar point and a GRID
// cannot have the same ID). The same point listed twice in a deck is one point.
class ScalarPointTable final : public TypedTable<ScalarPointTable, ScalarPointCard>
{
public:
    static constexpr std::string_view NAME = "ScalarPoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node, true};
    using Format = ScalarPointFormat;

    [[nodiscard]] static base::EntityId cardId(const ScalarPointCard& card) noexcept { return card.id; }
    std::uint32_t append(const ScalarPointCard& card, base::CardKey key);
    [[nodiscard]] ScalarPointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ScalarPointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}
    // Consecutive IDs are written as "first THRU last" when the model is written with compress_thru.
    [[nodiscard]] bool writesRuns() const noexcept override { return true; }
    [[nodiscard]] bool continuesRun(std::uint32_t previous, std::uint32_t row) const override;
    void writeRun(std::uint32_t row, std::uint32_t count, FieldWriter& writer) const override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
};

} // namespace oor::model
