#pragma once

#include "oor/model/cards/properties/compPropExplicit/CompPropExplicitCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CompPropExplicitFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CompPropExplicitCard& card);
    [[nodiscard]] static std::optional<CompPropExplicitCard> read(FieldReader& reader);
    static void write(const CompPropExplicitCard& card, FieldWriter& writer);
};

// All the PCOMPA cards. The ID space is the card type's own: the ID is the one of the PCOMP that the card extends, so
// it is also a reference to a property (renumbering the properties follows into it). The cards are kept whole
// (CardRows); the query engine reads the PID.
class CompPropExplicitTable final : public TypedTable<CompPropExplicitTable, CompPropExplicitCard>
{
public:
    static constexpr std::string_view NAME = "CompPropExplicit";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = CompPropExplicitFormat;

    [[nodiscard]] static base::EntityId cardId(const CompPropExplicitCard& card) noexcept { return card.id; }
    std::uint32_t append(const CompPropExplicitCard& card, base::CardKey key);
    [[nodiscard]] CompPropExplicitCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CompPropExplicitCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CompPropExplicitCard> m_cards;
};

} // namespace oor::model
