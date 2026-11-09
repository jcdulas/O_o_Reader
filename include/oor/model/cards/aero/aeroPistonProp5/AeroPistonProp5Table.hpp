#pragma once

#include "oor/model/cards/aero/aeroPistonProp5/AeroPistonProp5Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroPistonProp5Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroPistonProp5Card& card);
    [[nodiscard]] static std::optional<AeroPistonProp5Card> read(FieldReader& reader);
    static void write(const AeroPistonProp5Card& card, FieldWriter& writer);
};

// All the PAERO5 cards. The PID is in the space of the aerodynamic properties (IdSpace::AeroProperty, category
// AeroProperty), which PAERO1 to PAERO5 share: a CAERO5 names it (ReferenceTarget::AeroProperty). LALPHA, LXIS and
// LTAUS are AEFACT cards (ReferenceTarget::AeroFactor). The card is kept whole: the strips are a list that nobody reads
// as a column.
class AeroPistonProp5Table final : public TypedTable<AeroPistonProp5Table, AeroPistonProp5Card>
{
public:
    static constexpr std::string_view NAME = "AeroPistonProp5";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroProperty, base::IdSpace::AeroProperty};
    using Format = AeroPistonProp5Format;

    [[nodiscard]] static base::EntityId cardId(const AeroPistonProp5Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroPistonProp5Card& card, base::CardKey key);
    [[nodiscard]] AeroPistonProp5Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroPistonProp5Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroPistonProp5Card> m_cards;
};

} // namespace oor::model
