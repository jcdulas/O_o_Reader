#pragma once

#include "oor/model/cards/properties/compSolidProp/CompSolidPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CompSolidPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CompSolidPropCard& card);
    [[nodiscard]] static std::optional<CompSolidPropCard> read(FieldReader& reader);
    static void write(const CompSolidPropCard& card, FieldWriter& writer);
};

// All the PCOMPS cards, kept whole next to a column of their PIDs. A PCOMPS is a property (IdSpace::Property): the PID
// of a solid element finds it, and it shares its number with the PSOLID, PCOMPLS and every other property.
class CompSolidPropTable final : public TypedTable<CompSolidPropTable, CompSolidPropCard>
{
public:
    static constexpr std::string_view NAME = "CompSolidProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CompSolidPropFormat;

    [[nodiscard]] static base::EntityId cardId(const CompSolidPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const CompSolidPropCard& card, base::CardKey key);
    [[nodiscard]] CompSolidPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CompSolidPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CompSolidPropCard> m_cards;
};

} // namespace oor::model
