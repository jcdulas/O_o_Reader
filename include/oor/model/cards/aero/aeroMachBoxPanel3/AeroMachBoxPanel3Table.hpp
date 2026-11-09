#pragma once

#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroMachBoxPanel3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroMachBoxPanel3Card& card);
    [[nodiscard]] static std::optional<AeroMachBoxPanel3Card> read(FieldReader& reader);
    static void write(const AeroMachBoxPanel3Card& card, FieldWriter& writer);
};

// All the CAERO3 cards, kept whole next to a column of their EIDs. The EID is in the space of the aerodynamic elements
// (IdSpace::AeroElement, category AeroElement) that CAERO1 to CAERO5 share. The PID is a PAERO3
// (ReferenceTarget::AeroProperty), CP a coordinate system, LISTW, LISTC1 and LISTC2 AEFACT cards
// (ReferenceTarget::AeroFactor).
class AeroMachBoxPanel3Table final : public TypedTable<AeroMachBoxPanel3Table, AeroMachBoxPanel3Card>
{
public:
    static constexpr std::string_view NAME = "AeroMachBoxPanel3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroElement, base::IdSpace::AeroElement};
    using Format = AeroMachBoxPanel3Format;

    [[nodiscard]] static base::EntityId cardId(const AeroMachBoxPanel3Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroMachBoxPanel3Card& card, base::CardKey key);
    [[nodiscard]] AeroMachBoxPanel3Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroMachBoxPanel3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroMachBoxPanel3Card> m_cards;
};

} // namespace oor::model
