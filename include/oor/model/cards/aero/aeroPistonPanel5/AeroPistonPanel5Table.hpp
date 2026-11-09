#pragma once

#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroPistonPanel5Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroPistonPanel5Card& card);
    [[nodiscard]] static std::optional<AeroPistonPanel5Card> read(FieldReader& reader);
    static void write(const AeroPistonPanel5Card& card, FieldWriter& writer);
};

// All the CAERO5 cards, kept whole next to a column of their EIDs. The EID is in the space of the aerodynamic elements
// (IdSpace::AeroElement, category AeroElement) that CAERO1 to CAERO5 share. The PID is a PAERO5
// (ReferenceTarget::AeroProperty), CP a coordinate system, LSPAN and NTHICK AEFACT cards (ReferenceTarget::AeroFactor).
class AeroPistonPanel5Table final : public TypedTable<AeroPistonPanel5Table, AeroPistonPanel5Card>
{
public:
    static constexpr std::string_view NAME = "AeroPistonPanel5";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroElement, base::IdSpace::AeroElement};
    using Format = AeroPistonPanel5Format;

    [[nodiscard]] static base::EntityId cardId(const AeroPistonPanel5Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroPistonPanel5Card& card, base::CardKey key);
    [[nodiscard]] AeroPistonPanel5Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroPistonPanel5Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroPistonPanel5Card> m_cards;
};

} // namespace oor::model
