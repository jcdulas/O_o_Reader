#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqList/NonlinearHarmonicFreqListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearHarmonicFreqListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearHarmonicFreqListCard& card);
    [[nodiscard]] static std::optional<NonlinearHarmonicFreqListCard> read(FieldReader& reader);
    static void write(const NonlinearHarmonicFreqListCard& card, FieldWriter& writer);
};

// All the NLFREQ cards. The ID is unique among them (IdSpace::PerCardType) and numbers, in the category
// NonlinearFrequency, the same selection as the NLFREQ1 and NLFREQC cards: an NLHARM names any of the three, and the
// format reference allows one entry of the category for an ID. The frequencies of all the cards are one list in CSR
// form.
class NonlinearHarmonicFreqListTable final
    : public TypedTable<NonlinearHarmonicFreqListTable, NonlinearHarmonicFreqListCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearHarmonicFreqList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearFrequency, base::IdSpace::PerCardType};
    using Format = NonlinearHarmonicFreqListFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearHarmonicFreqListCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearHarmonicFreqListCard& card, base::CardKey key);
    [[nodiscard]] NonlinearHarmonicFreqListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearHarmonicFreqListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an NLFREQ refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& frequenciesColumn() noexcept { return m_frequencies; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_frequencies;
};

} // namespace oor::model
