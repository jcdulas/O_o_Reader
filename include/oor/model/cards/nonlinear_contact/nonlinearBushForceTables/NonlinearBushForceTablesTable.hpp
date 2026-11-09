#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearBushForceTables/NonlinearBushForceTablesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearBushForceTablesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearBushForceTablesCard& card);
    [[nodiscard]] static std::optional<NonlinearBushForceTablesCard> read(FieldReader& reader);
    static void write(const NonlinearBushForceTablesCard& card, FieldWriter& writer);
};

// All the LBSH3DG cards, each kept whole (CardRows: a matrix of 36 table IDs is not a set of columns) next to the
// column of its LID, which is what the query engine reads. The LID is the number an NLBSH3D names
// (ReferenceTarget::BushingForceSet, category BushingForceSet) and is unique among the LBSH3DG: the ID space is the
// card type's own. The terms are TABLED5 entries in the space of the dynamic tables.
class NonlinearBushForceTablesTable final
    : public TypedTable<NonlinearBushForceTablesTable, NonlinearBushForceTablesCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearBushForceTables";
    static inline const CardTypeInfo INFO{NAME, CardCategory::BushingForceSet, base::IdSpace::PerCardType};
    using Format = NonlinearBushForceTablesFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearBushForceTablesCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearBushForceTablesCard& card, base::CardKey key);
    [[nodiscard]] NonlinearBushForceTablesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearBushForceTablesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<NonlinearBushForceTablesCard> m_cards;
};

} // namespace oor::model
