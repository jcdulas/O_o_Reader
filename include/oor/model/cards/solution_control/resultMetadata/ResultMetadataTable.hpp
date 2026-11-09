#pragma once

#include "oor/model/cards/solution_control/resultMetadata/ResultMetadataCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ResultMetadataFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ResultMetadataCard& card);
    [[nodiscard]] static std::optional<ResultMetadataCard> read(FieldReader& reader);
    static void write(const ResultMetadataCard& card, FieldWriter& writer);
};

// All the METADATA cards (no ID: IdSpace::None, category Other). The handle (META) identifies a card; a deck may have
// one per superelement and the format reference does not say that two cards of one handle are an error. The card names
// no other card. It is kept whole next to a column of its handles.
class ResultMetadataTable final : public TypedTable<ResultMetadataTable, ResultMetadataCard>
{
public:
    static constexpr std::string_view NAME = "ResultMetadata";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ResultMetadataFormat;

    [[nodiscard]] static base::EntityId cardId(const ResultMetadataCard&) noexcept { return 0; }
    std::uint32_t append(const ResultMetadataCard& card, base::CardKey key);
    [[nodiscard]] ResultMetadataCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ResultMetadataCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    // The handle of a row, without copying the card.
    [[nodiscard]] std::string handleAt(std::uint32_t row) const { return m_handle.get(row); }
    [[nodiscard]] Column<std::string>& handleColumn() noexcept { return m_handle; }

private:
    Column<std::string> m_handle;
    CardRows<ResultMetadataCard> m_cards;
};

} // namespace oor::model
