#pragma once

#include "oor/model/cards/coordinate_systems/cylindricalFrame2/CylindricalFrame2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct CylindricalFrame2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CylindricalFrame2Card& card);
    [[nodiscard]] static std::optional<CylindricalFrame2Card> read(FieldReader& reader);
    static void write(const CylindricalFrame2Card& card, FieldWriter& writer);
};

// All the CORD2C systems. The three points are one column of (x, y, z) triples each: (N, 3) NumPy views without copy.
class CylindricalFrame2Table final : public TypedTable<CylindricalFrame2Table, CylindricalFrame2Card>
{
public:
    static constexpr std::string_view NAME = "CylindricalFrame2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Coordinate, base::IdSpace::Coordinate};
    using Format = CylindricalFrame2Format;

    CylindricalFrame2Table() : m_referenceFrameId(0) {}

    [[nodiscard]] static base::EntityId cardId(const CylindricalFrame2Card& card) noexcept { return card.id; }
    std::uint32_t append(const CylindricalFrame2Card& card, base::CardKey key);
    [[nodiscard]] CylindricalFrame2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const CylindricalFrame2Card& card);
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
    [[nodiscard]] Column<base::EntityId>& referenceFrameIdColumn() noexcept { return m_referenceFrameId; }
    [[nodiscard]] Column<std::array<double, 3>>& pointAColumn() noexcept { return m_pointA; }
    [[nodiscard]] Column<std::array<double, 3>>& pointBColumn() noexcept { return m_pointB; }
    [[nodiscard]] Column<std::array<double, 3>>& pointCColumn() noexcept { return m_pointC; }

private:
    std::uint64_t m_version = 0;
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_referenceFrameId;
    Column<std::array<double, 3>> m_pointA;
    Column<std::array<double, 3>> m_pointB;
    Column<std::array<double, 3>> m_pointC;
};

} // namespace oor::model
