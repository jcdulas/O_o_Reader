#pragma once

#include "oor/model/cards/nodes/axisymConeControl/AxisymConeControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConeControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConeControlCard& card);
    [[nodiscard]] static std::optional<AxisymConeControlCard> read(FieldReader& reader);
    static void write(const AxisymConeControlCard& card, FieldWriter& writer);
};

// All the AXIC cards. Only one is allowed in a deck; the card has no ID (IdSpace::None, category Other).
class AxisymConeControlTable final : public TypedTable<AxisymConeControlTable, AxisymConeControlCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConeControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AxisymConeControlFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymConeControlCard&) noexcept { return 0; }
    std::uint32_t append(const AxisymConeControlCard& card, base::CardKey key);
    [[nodiscard]] AxisymConeControlCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymConeControlCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an AXIC refers to no card
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::int32_t>& highestHarmonicColumn() noexcept { return m_highestHarmonic; }

private:
    Column<std::int32_t> m_highestHarmonic;
};

} // namespace oor::model
