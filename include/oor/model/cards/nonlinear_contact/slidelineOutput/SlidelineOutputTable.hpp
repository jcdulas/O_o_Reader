#pragma once

#include "oor/model/cards/nonlinear_contact/slidelineOutput/SlidelineOutputCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SlidelineOutputFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SlidelineOutputCard& card);
    [[nodiscard]] static std::optional<SlidelineOutputCard> read(FieldReader& reader);
    static void write(const SlidelineOutputCard& card, FieldWriter& writer);
};

// All the BOUTPUT cards. The ID names a BCONP region or a contact grid, so it is not an ID of this type's own:
// IdSpace::Group (several cards may give nodes of the same region), category Other.
class SlidelineOutputTable final : public TypedTable<SlidelineOutputTable, SlidelineOutputCard>
{
public:
    static constexpr std::string_view NAME = "SlidelineOutput";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = SlidelineOutputFormat;

    [[nodiscard]] static base::EntityId cardId(const SlidelineOutputCard& card) noexcept { return card.id; }
    std::uint32_t append(const SlidelineOutputCard& card, base::CardKey key);
    [[nodiscard]] SlidelineOutputCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SlidelineOutputCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& allColumn() noexcept { return m_all; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_all;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
