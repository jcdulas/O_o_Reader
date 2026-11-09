#pragma once

#include "oor/model/cards/loads/acousticInfiniteParams/AcousticInfiniteParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticInfiniteParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticInfiniteParamsCard& card);
    [[nodiscard]] static std::optional<AcousticInfiniteParamsCard> read(FieldReader& reader);
    static void write(const AcousticInfiniteParamsCard& card, FieldWriter& writer);
};

// All the ACIFPRM cards (no ID: IdSpace::None, category Other). The format reference allows one in a deck: a
// second one is kept like the first and reported when the deck is read, the first being the one in force. The card
// names no other card. It is kept whole: its pairs of name and value are a list that nobody reads as columns.
class AcousticInfiniteParamsTable final : public TypedTable<AcousticInfiniteParamsTable, AcousticInfiniteParamsCard>
{
public:
    static constexpr std::string_view NAME = "AcousticInfiniteParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AcousticInfiniteParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticInfiniteParamsCard&) noexcept { return 0; }
    std::uint32_t append(const AcousticInfiniteParamsCard& card, base::CardKey key);
    [[nodiscard]] AcousticInfiniteParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AcousticInfiniteParamsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}
    // Only one ACIFPRM entry is allowed: a second one is reported.
    void finishRead(Model& model, const ReadContext& context) override;

private:
    CardRows<AcousticInfiniteParamsCard> m_cards;
};

} // namespace oor::model
