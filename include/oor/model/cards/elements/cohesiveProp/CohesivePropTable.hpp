#pragma once

#include "oor/model/cards/elements/cohesiveProp/CohesivePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CohesivePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CohesivePropCard& card);
    [[nodiscard]] static std::optional<CohesivePropCard> read(FieldReader& reader);
    static void write(const CohesivePropCard& card, FieldWriter& writer);
};

// All the PCOHE properties: the ID is in the space every property type shares (IdSpace::Property), so the PID of a
// CIFQUAD, CIFHEX, CIFPENT or CIFQDX finds it; the MID is a material (an MCOHE).
class CohesivePropTable final : public TypedTable<CohesivePropTable, CohesivePropCard>
{
public:
    static constexpr std::string_view NAME = "CohesiveProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CohesivePropFormat;

    [[nodiscard]] static base::EntityId cardId(const CohesivePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const CohesivePropCard& card, base::CardKey key);
    [[nodiscard]] CohesivePropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CohesivePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CohesivePropCard> m_cards;
};

} // namespace oor::model
