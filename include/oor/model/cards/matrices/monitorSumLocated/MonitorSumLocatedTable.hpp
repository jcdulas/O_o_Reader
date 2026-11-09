#pragma once

#include "oor/model/cards/matrices/monitorSumLocated/MonitorSumLocatedCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorSumLocatedFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorSumLocatedCard& card);
    [[nodiscard]] static std::optional<MonitorSumLocatedCard> read(FieldReader& reader);
    static void write(const MonitorSumLocatedCard& card, FieldWriter& writer);
};

// All the MONSUM1 cards, in the category MonitorPoint. A monitor result has no numeric ID (IdSpace::None): it is found
// by its NAME. CP and CD are references to coordinate systems; the names of the monitor results it sums are not IDs.
// The cards are kept whole next to a column of their names.
class MonitorSumLocatedTable final : public TypedTable<MonitorSumLocatedTable, MonitorSumLocatedCard>
{
public:
    static constexpr std::string_view NAME = "MonitorSumLocated";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorSumLocatedFormat;

    [[nodiscard]] static base::EntityId cardId(const MonitorSumLocatedCard&) noexcept { return 0; }
    std::uint32_t append(const MonitorSumLocatedCard& card, base::CardKey key);
    [[nodiscard]] MonitorSumLocatedCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MonitorSumLocatedCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The name of a row, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_name.get(row); }
    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }

private:
    Column<std::string> m_name;
    CardRows<MonitorSumLocatedCard> m_cards;
};

} // namespace oor::model
