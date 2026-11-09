#pragma once

#include "oor/model/cards/thermal/axisymFluid2/AxisymFluid2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluid2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluid2Card& card);
    [[nodiscard]] static std::optional<AxisymFluid2Card> read(FieldReader& reader);
    static void write(const AxisymFluid2Card& card, FieldWriter& writer);
};

// All the CFLUID2 cards. The ID is an element ID, in the space every element shares (IdSpace::Element). The absent RHO
// and B are NaN.
class AxisymFluid2Table final : public TypedTable<AxisymFluid2Table, AxisymFluid2Card>
{
public:
    static constexpr std::string_view NAME = "AxisymFluid2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AxisymFluid2Format;

    AxisymFluid2Table();

    [[nodiscard]] static base::EntityId cardId(const AxisymFluid2Card& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymFluid2Card& card, base::CardKey key);
    [[nodiscard]] AxisymFluid2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymFluid2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the RINGFL cards are not typed

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, AxisymFluid2Card::RING_COUNT>>& ringsColumn() noexcept
    {
        return m_rings;
    }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& bulkModulusColumn() noexcept { return m_bulkModulus; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, AxisymFluid2Card::RING_COUNT>> m_rings;
    Column<double> m_density;
    Column<double> m_bulkModulus;
};

} // namespace oor::model
