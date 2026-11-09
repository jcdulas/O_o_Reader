#pragma once

#include "oor/model/cards/tables/tableStressTemperature/TableStressTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableStressTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableStressTemperatureCard& card);
    [[nodiscard]] static std::optional<TableStressTemperatureCard> read(FieldReader& reader);
    static void write(const TableStressTemperatureCard& card, FieldWriter& writer);
};

// All the TABLEST cards. The ID is in the stress table space it shares with TABLES1 (IdSpace::StressTable), and the
// table IDs of the pairs are references to TABLES1 cards of that space. The temperatures and the table IDs of every
// card are two lists in CSR form.
class TableStressTemperatureTable final : public TypedTable<TableStressTemperatureTable, TableStressTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "TableStressTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::StressTable};
    using Format = TableStressTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const TableStressTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableStressTemperatureCard& card, base::CardKey key);
    [[nodiscard]] TableStressTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableStressTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& temperaturesColumn() noexcept { return m_temperatures; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& tablesColumn() noexcept { return m_tables; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_temperatures;
    VariableLengthColumn<base::EntityId> m_tables;
};

} // namespace oor::model
