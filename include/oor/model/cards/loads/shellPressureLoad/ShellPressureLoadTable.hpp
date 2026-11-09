#pragma once

#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ShellPressureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ShellPressureLoadCard& card);
    [[nodiscard]] static std::optional<ShellPressureLoadCard> read(FieldReader& reader);
    static void write(const ShellPressureLoadCard& card, FieldWriter& writer);
};

// All the PLOAD2 cards. The ID is a load set ID shared by many cards (IdSpace::Group). The element lists are one CSR
// column of (first, last) runs.
class ShellPressureLoadTable final : public TypedTable<ShellPressureLoadTable, ShellPressureLoadCard>
{
public:
    static constexpr std::string_view NAME = "ShellPressureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = ShellPressureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const ShellPressureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const ShellPressureLoadCard& card, base::CardKey key);
    [[nodiscard]] ShellPressureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ShellPressureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    // The pressure and the element runs (first, last) of one card, without making the card: what the load queries
    // read of millions of cards.
    [[nodiscard]] double pressureAt(std::uint32_t row) const { return m_pressure.get(row); }
    [[nodiscard]] std::span<const std::array<base::EntityId, 2>> runsAt(std::uint32_t row) const
    {
        return m_runs.get(row);
    }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& pressureColumn() noexcept { return m_pressure; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_pressure;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
