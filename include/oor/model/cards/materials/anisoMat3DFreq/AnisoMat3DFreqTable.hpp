#pragma once

#include "oor/model/cards/materials/anisoMat3DFreq/AnisoMat3DFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AnisoMat3DFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AnisoMat3DFreqCard& card);
    [[nodiscard]] static std::optional<AnisoMat3DFreqCard> read(FieldReader& reader);
    static void write(const AnisoMat3DFreqCard& card, FieldWriter& writer);
};

// All the MAT9F cards. The ID space is the card type's own: the ID is the one of the MAT9 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEDi cards (ReferenceTarget::DynamicTable). The cards are kept whole (CardRows); the query engine reads the ID
// only.
class AnisoMat3DFreqTable final : public TypedTable<AnisoMat3DFreqTable, AnisoMat3DFreqCard>
{
public:
    static constexpr std::string_view NAME = "AnisoMat3DFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = AnisoMat3DFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const AnisoMat3DFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const AnisoMat3DFreqCard& card, base::CardKey key);
    [[nodiscard]] AnisoMat3DFreqCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AnisoMat3DFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AnisoMat3DFreqCard> m_cards;
};

} // namespace oor::model
