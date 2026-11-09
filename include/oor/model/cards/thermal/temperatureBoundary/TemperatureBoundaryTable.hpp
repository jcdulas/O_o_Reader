#pragma once

#include "oor/model/cards/thermal/temperatureBoundary/TemperatureBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct TemperatureBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TemperatureBoundaryCard& card);
    [[nodiscard]] static std::optional<TemperatureBoundaryCard> read(FieldReader& reader);
    static void write(const TemperatureBoundaryCard& card, FieldWriter& writer);
};

// All the TEMPBC cards. The ID is a set ID shared by many cards (IdSpace::Group). The category is Constraint: a STAT
// set is selected by `SPC = SID`, so it resolves as a constraint set. A TRAN set is the EXCITEID of a
// TLOADi: the table says it defines that excitation set (CardTable::definesExcitationIds). The temperatures are one CSR
// column and the grid points one CSR column of (first, last, by) runs, one run per temperature.
class TemperatureBoundaryTable final : public TypedTable<TemperatureBoundaryTable, TemperatureBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "TemperatureBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = TemperatureBoundaryFormat;

    TemperatureBoundaryTable();

    [[nodiscard]] static base::EntityId cardId(const TemperatureBoundaryCard& card) noexcept { return card.id; }
    std::uint32_t append(const TemperatureBoundaryCard& card, base::CardKey key);
    [[nodiscard]] TemperatureBoundaryCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TemperatureBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override
    {
        return m_kind.get(row) == "Transient" ? m_id.get(row) : 0;
    }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<double>& temperaturesColumn() noexcept { return m_temperatures; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_kind;
    VariableLengthColumn<double> m_temperatures;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
