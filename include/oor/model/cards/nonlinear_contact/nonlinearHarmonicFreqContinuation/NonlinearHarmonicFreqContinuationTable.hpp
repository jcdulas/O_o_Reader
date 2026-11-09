#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqContinuation/NonlinearHarmonicFreqContinuationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearHarmonicFreqContinuationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearHarmonicFreqContinuationCard& card);
    [[nodiscard]] static std::optional<NonlinearHarmonicFreqContinuationCard> read(FieldReader& reader);
    static void write(const NonlinearHarmonicFreqContinuationCard& card, FieldWriter& writer);
};

// All the NLFREQC cards. The ID is unique among them (IdSpace::PerCardType) and numbers, in the category
// NonlinearFrequency, the same selection as the NLFREQ and NLFREQ1 cards. The cards are kept whole (CardRows); the
// query engine reads the ID only.
class NonlinearHarmonicFreqContinuationTable final
    : public TypedTable<NonlinearHarmonicFreqContinuationTable, NonlinearHarmonicFreqContinuationCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearHarmonicFreqContinuation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearFrequency, base::IdSpace::PerCardType};
    using Format = NonlinearHarmonicFreqContinuationFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearHarmonicFreqContinuationCard& card) noexcept
    {
        return card.id;
    }
    std::uint32_t append(const NonlinearHarmonicFreqContinuationCard& card, base::CardKey key);
    [[nodiscard]] NonlinearHarmonicFreqContinuationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearHarmonicFreqContinuationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an NLFREQC refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<NonlinearHarmonicFreqContinuationCard> m_cards;
};

} // namespace oor::model
