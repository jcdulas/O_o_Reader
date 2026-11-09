#pragma once

#include "oor/model/cards/explicit/constantSpallation/ConstantSpallationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ConstantSpallationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ConstantSpallationCard& card);
    [[nodiscard]] static std::optional<ConstantSpallationCard> read(FieldReader& reader);
    static void write(const ConstantSpallationCard& card, FieldWriter& writer);
};

// All the PMINC cards. The ID is the number of a spall model, unique among the PMINC cards (IdSpace::PerCardType,
// category SpallModel: a spall model is not a property, whatever the name of its field says). A MATDEUL names the
// model. The cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class ConstantSpallationTable final : public TypedTable<ConstantSpallationTable, ConstantSpallationCard>
{
public:
    static constexpr std::string_view NAME = "ConstantSpallation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::SpallModel, base::IdSpace::PerCardType};
    using Format = ConstantSpallationFormat;

    [[nodiscard]] static base::EntityId cardId(const ConstantSpallationCard& card) noexcept { return card.id; }
    std::uint32_t append(const ConstantSpallationCard& card, base::CardKey key);
    [[nodiscard]] ConstantSpallationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ConstantSpallationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PMINC refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ConstantSpallationCard> m_cards;
};

} // namespace oor::model
