#pragma once

#include "oor/model/cards/loads/metadataBooleanPairs/MetadataBooleanPairsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MetadataBooleanPairsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MetadataBooleanPairsCard& card);
    [[nodiscard]] static std::optional<MetadataBooleanPairsCard> read(FieldReader& reader);
    static void write(const MetadataBooleanPairsCard& card, FieldWriter& writer);
};

// All the ABSNMVB cards. The ID is the ID of the meta data group, shared by the cards that belong to it
// (IdSpace::Group, category Other: the group itself is the ABSTRCT cards, category MetadataGroup), and a reference to
// them. The cards are kept whole next to a column of their IDs.
class MetadataBooleanPairsTable final : public TypedTable<MetadataBooleanPairsTable, MetadataBooleanPairsCard>
{
public:
    static constexpr std::string_view NAME = "MetadataBooleanPairs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = MetadataBooleanPairsFormat;

    [[nodiscard]] static base::EntityId cardId(const MetadataBooleanPairsCard& card) noexcept
    {
        return card.metadataGroupId;
    }
    std::uint32_t append(const MetadataBooleanPairsCard& card, base::CardKey key);
    [[nodiscard]] MetadataBooleanPairsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MetadataBooleanPairsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_metadataGroupId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& metadataGroupIdColumn() noexcept { return m_metadataGroupId; }

private:
    Column<base::EntityId> m_metadataGroupId;
    CardRows<MetadataBooleanPairsCard> m_cards;
};

} // namespace oor::model
