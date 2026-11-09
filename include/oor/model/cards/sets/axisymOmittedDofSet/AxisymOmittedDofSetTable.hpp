#pragma once

#include "oor/model/cards/sets/axisymOmittedDofSet/AxisymOmittedDofSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymOmittedDofSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymOmittedDofSetCard& card);
    [[nodiscard]] static std::optional<AxisymOmittedDofSetCard> read(FieldReader& reader);
    static void write(const AxisymOmittedDofSetCard& card, FieldWriter& writer);
};

// All the OMITAX cards, category Set, no ID (IdSpace::None, like the other sets of degrees of freedom). They are kept
// whole next to a column of their number of degrees of freedom. The rings are RINGAX cards, points of the node ID
// space.
class AxisymOmittedDofSetTable final : public TypedTable<AxisymOmittedDofSetTable, AxisymOmittedDofSetCard>
{
public:
    static constexpr std::string_view NAME = "AxisymOmittedDofSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = AxisymOmittedDofSetFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymOmittedDofSetCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymOmittedDofSetCard& card, base::CardKey key);
    [[nodiscard]] AxisymOmittedDofSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AxisymOmittedDofSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& countColumn() noexcept { return m_count; }

private:
    Column<std::int32_t> m_count;
    CardRows<AxisymOmittedDofSetCard> m_cards;
};

} // namespace oor::model
