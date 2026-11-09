#pragma once

#include "oor/model/cards/properties/compSolidPropNonlinear/CompSolidPropNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CompSolidPropNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CompSolidPropNonlinearCard& card);
    [[nodiscard]] static std::optional<CompSolidPropNonlinearCard> read(FieldReader& reader);
    static void write(const CompSolidPropNonlinearCard& card, FieldWriter& writer);
};

// All the PCOMPLS cards, kept whole next to a column of their PIDs. A PCOMPLS is a property (IdSpace::Property): the
// PID of a CHEXA finds it, and it shares its number with the PSOLID, PCOMPG, PLCOMP and every other property.
class CompSolidPropNonlinearTable final : public TypedTable<CompSolidPropNonlinearTable, CompSolidPropNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "CompSolidPropNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CompSolidPropNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const CompSolidPropNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const CompSolidPropNonlinearCard& card, base::CardKey key);
    [[nodiscard]] CompSolidPropNonlinearCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CompSolidPropNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CompSolidPropNonlinearCard> m_cards;
};

} // namespace oor::model
