#pragma once

#include "oor/model/cards/explicit/maxPlasticStrainFailure/MaxPlasticStrainFailureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MaxPlasticStrainFailureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MaxPlasticStrainFailureCard& card);
    [[nodiscard]] static std::optional<MaxPlasticStrainFailureCard> read(FieldReader& reader);
    static void write(const MaxPlasticStrainFailureCard& card, FieldWriter& writer);
};

// All the FAILMPS cards. The ID is the number of a failure model, unique among the cards of its category
// (IdSpace::PerCardType: the other FAILxxx cards share the category FailureModel, and a check reports an ID two of them
// have). A MATDEUL names the model. The cards are kept whole (CardRows) next to their ID: they are few, and the query
// engine reads the ID.
class MaxPlasticStrainFailureTable final : public TypedTable<MaxPlasticStrainFailureTable, MaxPlasticStrainFailureCard>
{
public:
    static constexpr std::string_view NAME = "MaxPlasticStrainFailure";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FailureModel, base::IdSpace::PerCardType};
    using Format = MaxPlasticStrainFailureFormat;

    [[nodiscard]] static base::EntityId cardId(const MaxPlasticStrainFailureCard& card) noexcept { return card.id; }
    std::uint32_t append(const MaxPlasticStrainFailureCard& card, base::CardKey key);
    [[nodiscard]] MaxPlasticStrainFailureCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MaxPlasticStrainFailureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a FAILMPS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<MaxPlasticStrainFailureCard> m_cards;
};

} // namespace oor::model
