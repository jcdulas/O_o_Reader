#pragma once

#include "oor/model/cards/explicit/couplingHoleToCoupling/CouplingHoleToCouplingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CouplingHoleToCouplingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CouplingHoleToCouplingCard& card);
    [[nodiscard]] static std::optional<CouplingHoleToCouplingCard> read(FieldReader& reader);
    static void write(const CouplingHoleToCouplingCard& card, FieldWriter& writer);
};

// All the PORFCPL cards: the PID is unique among them (IdSpace::PerCardType, category Porosity). A LEAKAGE names it;
// the CSID is a COUPLE and the MID a MATDEUL.
class CouplingHoleToCouplingTable final : public TypedTable<CouplingHoleToCouplingTable, CouplingHoleToCouplingCard>
{
public:
    static constexpr std::string_view NAME = "CouplingHoleToCoupling";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = CouplingHoleToCouplingFormat;

    [[nodiscard]] static base::EntityId cardId(const CouplingHoleToCouplingCard& card) noexcept { return card.id; }
    std::uint32_t append(const CouplingHoleToCouplingCard& card, base::CardKey key);
    [[nodiscard]] CouplingHoleToCouplingCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CouplingHoleToCouplingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<CouplingHoleToCouplingCard> m_cards;
};

} // namespace oor::model
