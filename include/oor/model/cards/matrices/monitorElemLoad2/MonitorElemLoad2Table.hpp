#pragma once

#include "oor/model/cards/matrices/monitorElemLoad2/MonitorElemLoad2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorElemLoad2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorElemLoad2Card& card);
    [[nodiscard]] static std::optional<MonitorElemLoad2Card> read(FieldReader& reader);
    static void write(const MonitorElemLoad2Card& card, FieldWriter& writer);
};

// All the MONPNT2 cards. A monitor point has no numeric ID (IdSpace::None): it is found by its NAME, which the format
// reference says is unique (a repeated NAME is reported when the deck is read).
class MonitorElemLoad2Table final : public TypedTable<MonitorElemLoad2Table, MonitorElemLoad2Card>
{
public:
    static constexpr std::string_view NAME = "MonitorElemLoad2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorElemLoad2Format;

    [[nodiscard]] static base::EntityId cardId(const MonitorElemLoad2Card&) noexcept { return 0; }
    std::uint32_t append(const MonitorElemLoad2Card& card, base::CardKey key);
    [[nodiscard]] MonitorElemLoad2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const MonitorElemLoad2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Two MONPNT2 cards of one NAME are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }
    [[nodiscard]] Column<std::string>& resultTableColumn() noexcept { return m_resultTable; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<std::string>& resultItemColumn() noexcept { return m_resultItem; }
    [[nodiscard]] Column<base::EntityId>& elemIdColumn() noexcept { return m_elemId; }

private:
    Column<std::string> m_name;
    Column<std::string> m_label;
    Column<std::string> m_resultTable;
    Column<std::string> m_kind;
    Column<std::string> m_resultItem;
    Column<base::EntityId> m_elemId;
};

} // namespace oor::model
