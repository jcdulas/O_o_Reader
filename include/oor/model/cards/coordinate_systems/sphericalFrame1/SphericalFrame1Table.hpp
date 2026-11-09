#pragma once

#include "oor/model/cards/coordinate_systems/sphericalFrame1/SphericalFrame1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SphericalFrame1Format
{
    // The first thing wrong with the values of a system, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SphericalFrame1Card& card);
    // One value for each system the card defines (one or two).
    [[nodiscard]] static std::vector<SphericalFrame1Card> readAll(FieldReader& reader);
    static void write(const SphericalFrame1Card& card, FieldWriter& writer);
};

// All the CORD1S systems, one row each (a card with two systems gives two rows).
class SphericalFrame1Table final : public TypedTable<SphericalFrame1Table, SphericalFrame1Card>
{
public:
    static constexpr std::string_view NAME = "SphericalFrame1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Coordinate, base::IdSpace::Coordinate};
    using Format = SphericalFrame1Format;

    [[nodiscard]] static base::EntityId cardId(const SphericalFrame1Card& card) noexcept { return card.id; }
    std::uint32_t append(const SphericalFrame1Card& card, base::CardKey key);
    [[nodiscard]] SphericalFrame1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const SphericalFrame1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id)
    {
        m_id.set(row, id);
        ++m_version;
    }
    // Counts the changes made to the systems (a card added, edited, renumbered or dropped by a compaction): the cache
    // of node positions compares it with the value it saw to know that no coordinate system changed.
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
