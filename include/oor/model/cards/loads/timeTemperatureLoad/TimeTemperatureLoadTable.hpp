#pragma once

#include "oor/model/cards/loads/timeTemperatureLoad/TimeTemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TimeTemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TimeTemperatureLoadCard& card);
    [[nodiscard]] static std::optional<TimeTemperatureLoadCard> read(FieldReader& reader);
    static void write(const TimeTemperatureLoadCard& card, FieldWriter& writer);
};

// All the TTEMP cards. The ID is a temperature set ID shared with the TEMP cards (IdSpace::Group, category Load), so
// that `TEMPERATURE(LOAD) = n` finds it. GROUP_ID is a TemperatureGroup reference (not visited when it is -1, the
// word for every grid point) and TID a DynamicTable reference.
class TimeTemperatureLoadTable final : public TypedTable<TimeTemperatureLoadTable, TimeTemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "TimeTemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = TimeTemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const TimeTemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const TimeTemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] TimeTemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TimeTemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& groupIdColumn() noexcept { return m_groupId; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_groupId;
    Column<base::EntityId> m_tableId;
};

} // namespace oor::model
