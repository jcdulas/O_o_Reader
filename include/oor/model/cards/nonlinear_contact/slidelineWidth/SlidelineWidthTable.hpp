#pragma once

#include "oor/model/cards/nonlinear_contact/slidelineWidth/SlidelineWidthCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SlidelineWidthFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SlidelineWidthCard& card);
    [[nodiscard]] static std::optional<SlidelineWidthCard> read(FieldReader& reader);
    static void write(const SlidelineWidthCard& card, FieldWriter& writer);
};

// All the BWIDTH cards. A BWIDTH goes with the BLSEG of its ID (a one to one correspondence), so the ID is a reference
// to the BLSEG (category SlidelineSegments) and is unique among the BWIDTH cards (IdSpace::PerCardType, category
// Other). The widths are one CSR column of (first, last, by) triples; a single width has by == 0.0.
class SlidelineWidthTable final : public TypedTable<SlidelineWidthTable, SlidelineWidthCard>
{
public:
    static constexpr std::string_view NAME = "SlidelineWidth";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = SlidelineWidthFormat;

    [[nodiscard]] static base::EntityId cardId(const SlidelineWidthCard& card) noexcept { return card.segmentsId; }
    std::uint32_t append(const SlidelineWidthCard& card, base::CardKey key);
    [[nodiscard]] SlidelineWidthCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SlidelineWidthCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_segmentsId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_segmentsId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& segmentsIdColumn() noexcept { return m_segmentsId; }
    [[nodiscard]] VariableLengthColumn<std::array<double, 3>>& widthsColumn() noexcept { return m_widths; }

private:
    Column<base::EntityId> m_segmentsId;
    VariableLengthColumn<std::array<double, 3>> m_widths;
};

} // namespace oor::model
