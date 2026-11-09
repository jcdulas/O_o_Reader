#pragma once

#include "oor/model/cards/dynamics/freqList/FreqListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FreqListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreqListCard& card);
    [[nodiscard]] static std::optional<FreqListCard> read(FieldReader& reader);
    static void write(const FreqListCard& card, FieldWriter& writer);
};

// All the FREQ cards. The ID is a frequency set ID shared by the FREQ, FREQ1 and FREQ2 cards (IdSpace::Group). The
// frequencies of every card are one list in CSR form.
class FreqListTable final : public TypedTable<FreqListTable, FreqListCard>
{
public:
    static constexpr std::string_view NAME = "FreqList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Frequency, base::IdSpace::Group};
    using Format = FreqListFormat;

    [[nodiscard]] static base::EntityId cardId(const FreqListCard& card) noexcept { return card.id; }
    std::uint32_t append(const FreqListCard& card, base::CardKey key);
    [[nodiscard]] FreqListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreqListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FREQ refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& frequenciesColumn() noexcept { return m_frequencies; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_frequencies;
};

} // namespace oor::model
