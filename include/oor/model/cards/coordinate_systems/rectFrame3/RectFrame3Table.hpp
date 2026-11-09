#pragma once

#include "oor/model/cards/coordinate_systems/rectFrame3/RectFrame3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct RectFrame3Format
{
    // The first thing wrong with the values of a system, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RectFrame3Card& card);
    // One value for each system the card defines (one or two).
    [[nodiscard]] static std::vector<RectFrame3Card> readAll(FieldReader& reader);
    static void write(const RectFrame3Card& card, FieldWriter& writer);
};

// All the CORD3R systems, one row each (a card with two systems gives two rows). Their IDs are in the space of every
// coordinate system (IdSpace::Coordinate).
class RectFrame3Table final : public TypedTable<RectFrame3Table, RectFrame3Card>
{
public:
    static constexpr std::string_view NAME = "RectFrame3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Coordinate, base::IdSpace::Coordinate};
    using Format = RectFrame3Format;

    [[nodiscard]] static base::EntityId cardId(const RectFrame3Card& card) noexcept { return card.id; }
    std::uint32_t append(const RectFrame3Card& card, base::CardKey key);
    [[nodiscard]] RectFrame3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const RectFrame3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id)
    {
        m_id.set(row, id);
        ++m_version;
    }
    // Counts the changes made to the systems (a card added, edited, renumbered or dropped by a compaction), like
    // RectFrame1Table::version: a cache of node positions compares it with the value it saw.
    [[nodiscard]] std::uint64_t version() const noexcept { return m_version; }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& nodesColumn() noexcept { return m_nodes; }

private:
    std::uint64_t m_version = 0;
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 3>> m_nodes;
};

} // namespace oor::model
