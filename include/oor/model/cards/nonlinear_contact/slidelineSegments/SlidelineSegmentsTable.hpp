#pragma once

#include "oor/model/cards/nonlinear_contact/slidelineSegments/SlidelineSegmentsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SlidelineSegmentsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SlidelineSegmentsCard& card);
    [[nodiscard]] static std::optional<SlidelineSegmentsCard> read(FieldReader& reader);
    static void write(const SlidelineSegmentsCard& card, FieldWriter& writer);
};

// All the BLSEG cards. The ID is unique among them (category SlidelineSegments), which is what the SECNDRY and PRIMARY
// of a BCONP and the ID of a BWIDTH name. The grid lists are one CSR column of (first, last, by) runs.
class SlidelineSegmentsTable final : public TypedTable<SlidelineSegmentsTable, SlidelineSegmentsCard>
{
public:
    static constexpr std::string_view NAME = "SlidelineSegments";
    static inline const CardTypeInfo INFO{NAME, CardCategory::SlidelineSegments, base::IdSpace::PerCardType};
    using Format = SlidelineSegmentsFormat;

    [[nodiscard]] static base::EntityId cardId(const SlidelineSegmentsCard& card) noexcept { return card.id; }
    std::uint32_t append(const SlidelineSegmentsCard& card, base::CardKey key);
    [[nodiscard]] SlidelineSegmentsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SlidelineSegmentsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
