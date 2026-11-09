#pragma once

#include "oor/model/cards/explicit/elasticShearModel/ElasticShearModelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ElasticShearModelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ElasticShearModelCard& card);
    [[nodiscard]] static std::optional<ElasticShearModelCard> read(FieldReader& reader);
    static void write(const ElasticShearModelCard& card, FieldWriter& writer);
};

// All the SHREL cards. The ID is the number of a shear model, unique among the cards of its category
// (IdSpace::PerCardType: the other SHRxxx cards share the category, and a check reports an ID two of them have). The
// cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class ElasticShearModelTable final : public TypedTable<ElasticShearModelTable, ElasticShearModelCard>
{
public:
    static constexpr std::string_view NAME = "ElasticShearModel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ShearModel, base::IdSpace::PerCardType};
    using Format = ElasticShearModelFormat;

    [[nodiscard]] static base::EntityId cardId(const ElasticShearModelCard& card) noexcept { return card.id; }
    std::uint32_t append(const ElasticShearModelCard& card, base::CardKey key);
    [[nodiscard]] ElasticShearModelCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ElasticShearModelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a SHREL refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ElasticShearModelCard> m_cards;
};

} // namespace oor::model
