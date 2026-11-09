#pragma once

#include "oor/model/cards/matrices/monitorSum/MonitorSumCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorSumFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorSumCard& card);
    [[nodiscard]] static std::optional<MonitorSumCard> read(FieldReader& reader);
    static void write(const MonitorSumCard& card, FieldWriter& writer);
};

// All the MONSUM cards, in the category MonitorPoint with the monitor points they add together. A monitor result has
// no numeric ID (IdSpace::None): it is found by its NAME, which the format reference asks to be unique among the
// monitor quantities unless the sum scales the result of its own name, so a repeated NAME is not reported by
// the table. The cards are kept whole next to a column of their names.
class MonitorSumTable final : public TypedTable<MonitorSumTable, MonitorSumCard>
{
public:
    static constexpr std::string_view NAME = "MonitorSum";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorSumFormat;

    [[nodiscard]] static base::EntityId cardId(const MonitorSumCard&) noexcept { return 0; }
    std::uint32_t append(const MonitorSumCard& card, base::CardKey key);
    [[nodiscard]] MonitorSumCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MonitorSumCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the names are not IDs

    // The name of a row, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_name.get(row); }
    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }

private:
    Column<std::string> m_name;
    CardRows<MonitorSumCard> m_cards;
};

} // namespace oor::model
