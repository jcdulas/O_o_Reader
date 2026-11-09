#pragma once

#include "oor/model/cards/aero/aeroFiniteBeamSpline7/AeroFiniteBeamSpline7Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct AeroFiniteBeamSpline7Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroFiniteBeamSpline7Card& card);
    [[nodiscard]] static std::optional<AeroFiniteBeamSpline7Card> read(FieldReader& reader);
    static void write(const AeroFiniteBeamSpline7Card& card, FieldWriter& writer);
};

// All the SPLINE7 cards, in the category AeroSpline (the number space of the splines is shared by SPLINE1 to SPLINE7
// and SPLINEX; IdSpace::PerCardType refuses a repeat inside this type and a check across the types reports the others).
// The CAERO is an aerodynamic element (ReferenceTarget::AeroElement) and may be blank, the AELIST an AELIST card
// (ReferenceTarget::AeroList), SETG a SET1 or SET3 (ReferenceTarget::Set) and CID a coordinate system. The cards are
// kept whole (CardRows) next to their EID: a SPLINE7 has a dozen fields of its own and few of them are ever set.
class AeroFiniteBeamSpline7Table final : public TypedTable<AeroFiniteBeamSpline7Table, AeroFiniteBeamSpline7Card>
{
public:
    static constexpr std::string_view NAME = "AeroFiniteBeamSpline7";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroSpline, base::IdSpace::PerCardType};
    using Format = AeroFiniteBeamSpline7Format;

    [[nodiscard]] static base::EntityId cardId(const AeroFiniteBeamSpline7Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroFiniteBeamSpline7Card& card, base::CardKey key);
    [[nodiscard]] AeroFiniteBeamSpline7Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroFiniteBeamSpline7Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroFiniteBeamSpline7Card> m_cards;
};

} // namespace oor::model
