#pragma once

#include "oor/model/cards/superelements/axisymConeSupport/AxisymConeSupportCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConeSupportFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConeSupportCard& card);
    [[nodiscard]] static std::optional<AxisymConeSupportCard> read(FieldReader& reader);
    static void write(const AxisymConeSupportCard& card, FieldWriter& writer);
};

// All the SUPAX cards, category Other, no ID (IdSpace::None). They are kept whole next to a column of their number of
// degrees of freedom.
class AxisymConeSupportTable final : public TypedTable<AxisymConeSupportTable, AxisymConeSupportCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConeSupport";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AxisymConeSupportFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymConeSupportCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymConeSupportCard& card, base::CardKey key);
    [[nodiscard]] AxisymConeSupportCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AxisymConeSupportCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // The rings are RINGAX cards, which are not typed: nothing to visit.
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<std::int32_t>& countColumn() noexcept { return m_count; }

private:
    Column<std::int32_t> m_count;
    CardRows<AxisymConeSupportCard> m_cards;
};

} // namespace oor::model
