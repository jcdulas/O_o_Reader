#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/elements/acousticSlot4/AcousticSlot4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticSlot4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticSlot4Card& card);
    [[nodiscard]] static std::optional<AcousticSlot4Card> read(FieldReader& reader);
    static void write(const AcousticSlot4Card& card, FieldWriter& writer);
};

// All the CSLOT4 elements: an element in the space every element shares (IdSpace::Element). The nodes are one column of
// four IDs (an (N, 4) NumPy view); a blank RHO or B is a NaN and a blank M is NO_SLOT_COUNT in its column.
class AcousticSlot4Table final : public TypedTable<AcousticSlot4Table, AcousticSlot4Card>
{
public:
    static constexpr std::string_view NAME = "AcousticSlot4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AcousticSlot4Format;
    static constexpr std::int32_t NO_SLOT_COUNT = -1; // the column's blank M (M is zero or more)

    AcousticSlot4Table() : m_density(BLANK_REAL), m_bulkModulus(BLANK_REAL), m_slotCount(NO_SLOT_COUNT) {}

    [[nodiscard]] static base::EntityId cardId(const AcousticSlot4Card& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticSlot4Card& card, base::CardKey key);
    [[nodiscard]] AcousticSlot4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticSlot4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, AcousticSlot4Card::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& bulkModulusColumn() noexcept { return m_bulkModulus; }
    [[nodiscard]] Column<std::int32_t>& slotCountColumn() noexcept { return m_slotCount; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, AcousticSlot4Card::NODE_COUNT>> m_nodes;
    Column<double> m_density;
    Column<double> m_bulkModulus;
    Column<std::int32_t> m_slotCount;
};

} // namespace oor::model
