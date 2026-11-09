#pragma once

#include "oor/model/cards/solution_control/resultFileParams/ResultFileParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ResultFileParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ResultFileParamsCard& card);
    [[nodiscard]] static std::optional<ResultFileParamsCard> read(FieldReader& reader);
    static void write(const ResultFileParamsCard& card, FieldWriter& writer);
};

// All the HDF5OUT cards (no ID: IdSpace::None, category Other). The card names no other card. It is kept whole
// (CardRows): its pairs of name and value are a list that nobody reads as columns.
class ResultFileParamsTable final : public TypedTable<ResultFileParamsTable, ResultFileParamsCard>
{
public:
    static constexpr std::string_view NAME = "ResultFileParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ResultFileParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ResultFileParamsCard&) noexcept { return 0; }
    std::uint32_t append(const ResultFileParamsCard& card, base::CardKey key);
    [[nodiscard]] ResultFileParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ResultFileParamsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

private:
    CardRows<ResultFileParamsCard> m_cards;
};

} // namespace oor::model
