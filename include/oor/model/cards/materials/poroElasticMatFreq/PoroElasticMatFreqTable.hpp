#pragma once

#include "oor/model/cards/materials/poroElasticMatFreq/PoroElasticMatFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PoroElasticMatFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PoroElasticMatFreqCard& card);
    [[nodiscard]] static std::optional<PoroElasticMatFreqCard> read(FieldReader& reader);
    static void write(const PoroElasticMatFreqCard& card, FieldWriter& writer);
};

// All the MATF1 cards. The ID space is the card type's own: the ID is the one of the MAT1 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable). The cards are kept whole (CardRows); the query engine reads the ID
// only.
class PoroElasticMatFreqTable final : public TypedTable<PoroElasticMatFreqTable, PoroElasticMatFreqCard>
{
public:
    static constexpr std::string_view NAME = "PoroElasticMatFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = PoroElasticMatFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const PoroElasticMatFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const PoroElasticMatFreqCard& card, base::CardKey key);
    [[nodiscard]] PoroElasticMatFreqCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PoroElasticMatFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PoroElasticMatFreqCard> m_cards;
};

} // namespace oor::model
