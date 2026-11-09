#pragma once

#include "oor/model/cards/matrices/monitorSumTotal/MonitorSumTotalCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorSumTotalFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorSumTotalCard& card);
    [[nodiscard]] static std::optional<MonitorSumTotalCard> read(FieldReader& reader);
    static void write(const MonitorSumTotalCard& card, FieldWriter& writer);
};

// All the MONSUMT cards, in the category MonitorPoint. A monitor result has no numeric ID (IdSpace::None): it is found
// by its NAME, which the format reference asks to be unique among the monitor quantities. CP and CD are references to
// coordinate systems; the names of the monitor results it sums are not IDs. The cards are kept whole next to a column
// of their names.
class MonitorSumTotalTable final : public TypedTable<MonitorSumTotalTable, MonitorSumTotalCard>
{
public:
    static constexpr std::string_view NAME = "MonitorSumTotal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorSumTotalFormat;

    [[nodiscard]] static base::EntityId cardId(const MonitorSumTotalCard&) noexcept { return 0; }
    std::uint32_t append(const MonitorSumTotalCard& card, base::CardKey key);
    [[nodiscard]] MonitorSumTotalCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MonitorSumTotalCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The name of a row, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_entryName.get(row); }
    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }

private:
    Column<std::string> m_entryName;
    CardRows<MonitorSumTotalCard> m_cards;
};

} // namespace oor::model
