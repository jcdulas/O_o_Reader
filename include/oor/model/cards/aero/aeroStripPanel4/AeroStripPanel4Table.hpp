#pragma once

#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroStripPanel4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroStripPanel4Card& card);
    [[nodiscard]] static std::optional<AeroStripPanel4Card> read(FieldReader& reader);
    static void write(const AeroStripPanel4Card& card, FieldWriter& writer);
};

// All the CAERO4 cards, kept whole next to a column of their EIDs. The EID is in the space of the aerodynamic elements
// (IdSpace::AeroElement, category AeroElement) that CAERO1 to CAERO5 share. The PID is a PAERO4
// (ReferenceTarget::AeroProperty), CP a coordinate system, LSPAN an AEFACT (ReferenceTarget::AeroFactor).
class AeroStripPanel4Table final : public TypedTable<AeroStripPanel4Table, AeroStripPanel4Card>
{
public:
    static constexpr std::string_view NAME = "AeroStripPanel4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroElement, base::IdSpace::AeroElement};
    using Format = AeroStripPanel4Format;

    [[nodiscard]] static base::EntityId cardId(const AeroStripPanel4Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroStripPanel4Card& card, base::CardKey key);
    [[nodiscard]] AeroStripPanel4Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroStripPanel4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroStripPanel4Card> m_cards;
};

} // namespace oor::model
