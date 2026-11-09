#pragma once

#include "oor/model/cards/nodes/axisymFluidControl/AxisymFluidControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymFluidControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymFluidControlCard& card);
    [[nodiscard]] static std::optional<AxisymFluidControlCard> read(FieldReader& reader);
    static void write(const AxisymFluidControlCard& card, FieldWriter& writer);
};

// All the AXIF cards. Only one is allowed in a deck; the card has no ID (IdSpace::None, category Other).
// They are kept whole next to a column of their number of harmonic items. The CID names a coordinate system.
class AxisymFluidControlTable final : public TypedTable<AxisymFluidControlTable, AxisymFluidControlCard>
{
public:
    static constexpr std::string_view NAME = "AxisymFluidControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AxisymFluidControlFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymFluidControlCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymFluidControlCard& card, base::CardKey key);
    [[nodiscard]] AxisymFluidControlCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AxisymFluidControlCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::int32_t>& harmonicCountColumn() noexcept { return m_harmonicCount; }

private:
    Column<std::int32_t> m_harmonicCount;
    CardRows<AxisymFluidControlCard> m_cards;
};

} // namespace oor::model
