#pragma once

#include "oor/model/cards/aero/aeroMachBoxProp3/AeroMachBoxProp3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroMachBoxProp3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroMachBoxProp3Card& card);
    [[nodiscard]] static std::optional<AeroMachBoxProp3Card> read(FieldReader& reader);
    static void write(const AeroMachBoxProp3Card& card, FieldWriter& writer);
};

// All the PAERO3 cards, kept whole next to a column of their PIDs. The PID is in the space the five PAERO cards share
// (IdSpace::AeroProperty, category AeroProperty): a CAERO3 names it (ReferenceTarget::AeroProperty). The card names no
// other card.
class AeroMachBoxProp3Table final : public TypedTable<AeroMachBoxProp3Table, AeroMachBoxProp3Card>
{
public:
    static constexpr std::string_view NAME = "AeroMachBoxProp3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroProperty, base::IdSpace::AeroProperty};
    using Format = AeroMachBoxProp3Format;

    [[nodiscard]] static base::EntityId cardId(const AeroMachBoxProp3Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroMachBoxProp3Card& card, base::CardKey key);
    [[nodiscard]] AeroMachBoxProp3Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroMachBoxProp3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroMachBoxProp3Card> m_cards;
};

} // namespace oor::model
