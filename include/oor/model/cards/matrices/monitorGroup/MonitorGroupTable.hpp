#pragma once

#include "oor/model/cards/matrices/monitorGroup/MonitorGroupCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorGroupFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorGroupCard& card);
    [[nodiscard]] static std::optional<MonitorGroupCard> read(FieldReader& reader);
    static void write(const MonitorGroupCard& card, FieldWriter& writer);
};

// All the MONGRP cards (category Other: a group is not a monitor point, and the monitor points of the model are the
// cards of the category MonitorPoint). The group has no numeric ID (IdSpace::None): it is found by its GNAME. The
// members are names of monitor points, not IDs. The cards are kept whole next to a column of their group names.
class MonitorGroupTable final : public TypedTable<MonitorGroupTable, MonitorGroupCard>
{
public:
    static constexpr std::string_view NAME = "MonitorGroup";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = MonitorGroupFormat;

    [[nodiscard]] static base::EntityId cardId(const MonitorGroupCard&) noexcept { return 0; }
    std::uint32_t append(const MonitorGroupCard& card, base::CardKey key);
    [[nodiscard]] MonitorGroupCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MonitorGroupCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the names are not IDs

    // The group name of a row, without copying the card.
    [[nodiscard]] std::string groupNameAt(std::uint32_t row) const { return m_groupName.get(row); }
    [[nodiscard]] Column<std::string>& groupNameColumn() noexcept { return m_groupName; }

private:
    Column<std::string> m_groupName;
    CardRows<MonitorGroupCard> m_cards;
};

} // namespace oor::model
