#pragma once

#include "oor/model/cards/materials/anisoMat2DFreq/AnisoMat2DFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AnisoMat2DFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AnisoMat2DFreqCard& card);
    [[nodiscard]] static std::optional<AnisoMat2DFreqCard> read(FieldReader& reader);
    static void write(const AnisoMat2DFreqCard& card, FieldWriter& writer);
};

// All the MAT2F cards. The ID space is the card type's own: the ID is the one of the MAT2 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEDi cards (ReferenceTarget::DynamicTable). The cards are kept whole (CardRows); the query engine reads the ID
// only.
class AnisoMat2DFreqTable final : public TypedTable<AnisoMat2DFreqTable, AnisoMat2DFreqCard>
{
public:
    static constexpr std::string_view NAME = "AnisoMat2DFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = AnisoMat2DFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const AnisoMat2DFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const AnisoMat2DFreqCard& card, base::CardKey key);
    [[nodiscard]] AnisoMat2DFreqCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AnisoMat2DFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AnisoMat2DFreqCard> m_cards;
};

} // namespace oor::model
