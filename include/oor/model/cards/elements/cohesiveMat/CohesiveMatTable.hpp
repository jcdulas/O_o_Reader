#pragma once

#include "oor/model/cards/elements/cohesiveMat/CohesiveMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CohesiveMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CohesiveMatCard& card);
    [[nodiscard]] static std::optional<CohesiveMatCard> read(FieldReader& reader);
    static void write(const CohesiveMatCard& card, FieldWriter& writer);
};

// All the MCOHE cards. An MCOHE is a material: its ID is in the space every material type shares (IdSpace::Material),
// so the cohesive elements' property and a MATUDS that names the material find it. Its TID is a stress table (TABLES1
// or TABLEST: ReferenceTarget::StressTable). The cards are kept whole (CardRows) next to their ID: they are few, and
// the query engine reads the ID.
class CohesiveMatTable final : public TypedTable<CohesiveMatTable, CohesiveMatCard>
{
public:
    static constexpr std::string_view NAME = "CohesiveMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = CohesiveMatFormat;

    [[nodiscard]] static base::EntityId cardId(const CohesiveMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const CohesiveMatCard& card, base::CardKey key);
    [[nodiscard]] CohesiveMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CohesiveMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CohesiveMatCard> m_cards;
};

} // namespace oor::model
