#pragma once

#include "oor/model/cards/loads/metadataRealPairs/MetadataRealPairsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MetadataRealPairsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MetadataRealPairsCard& card);
    [[nodiscard]] static std::optional<MetadataRealPairsCard> read(FieldReader& reader);
    static void write(const MetadataRealPairsCard& card, FieldWriter& writer);
};

// All the ABSNMVR cards. The ID is the ID of the meta data group, shared by the cards that belong to it
// (IdSpace::Group, category Other: the group itself is the ABSTRCT cards, category MetadataGroup), and a reference to
// them. The cards are kept whole next to a column of their IDs.
class MetadataRealPairsTable final : public TypedTable<MetadataRealPairsTable, MetadataRealPairsCard>
{
public:
    static constexpr std::string_view NAME = "MetadataRealPairs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = MetadataRealPairsFormat;

    [[nodiscard]] static base::EntityId cardId(const MetadataRealPairsCard& card) noexcept
    {
        return card.metadataGroupId;
    }
    std::uint32_t append(const MetadataRealPairsCard& card, base::CardKey key);
    [[nodiscard]] MetadataRealPairsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MetadataRealPairsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_metadataGroupId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& metadataGroupIdColumn() noexcept { return m_metadataGroupId; }

private:
    Column<base::EntityId> m_metadataGroupId;
    CardRows<MetadataRealPairsCard> m_cards;
};

} // namespace oor::model
