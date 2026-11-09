#pragma once

#include "oor/model/cards/materials/orthoMat2DFreq/OrthoMat2DFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct OrthoMat2DFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat2DFreqCard& card);
    [[nodiscard]] static std::optional<OrthoMat2DFreqCard> read(FieldReader& reader);
    static void write(const OrthoMat2DFreqCard& card, FieldWriter& writer);
};

// All the MAT8F cards. The ID space is the card type's own: the ID is the one of the MAT8 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEDi cards (ReferenceTarget::DynamicTable). The cards are kept whole (CardRows); the query engine reads the ID
// only.
class OrthoMat2DFreqTable final : public TypedTable<OrthoMat2DFreqTable, OrthoMat2DFreqCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat2DFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = OrthoMat2DFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat2DFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const OrthoMat2DFreqCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat2DFreqCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const OrthoMat2DFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<OrthoMat2DFreqCard> m_cards;
};

} // namespace oor::model
