#pragma once

#include "oor/model/cards/aero/aeroSplineRelaxation/AeroSplineRelaxationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroSplineRelaxationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroSplineRelaxationCard& card);
    [[nodiscard]] static std::optional<AeroSplineRelaxationCard> read(FieldReader& reader);
    static void write(const AeroSplineRelaxationCard& card, FieldWriter& writer);
};

// All the SPRELAX cards, in the category Other. A card has no ID (IdSpace::None): its SID1 names the spline it modifies
// and several cards may name the same one. SID1 and SID2 name splines (ReferenceTarget::AeroSpline), LIST2 and LIST1
// AELIST cards (ReferenceTarget::AeroList). The cards are kept whole (CardRows) next to a column of their SID1, which
// is what the query engine and the Arrow export see; a renumbering of the splines changes it with the card.
class AeroSplineRelaxationTable final : public TypedTable<AeroSplineRelaxationTable, AeroSplineRelaxationCard>
{
public:
    static constexpr std::string_view NAME = "AeroSplineRelaxation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AeroSplineRelaxationFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroSplineRelaxationCard&) noexcept { return 0; }
    std::uint32_t append(const AeroSplineRelaxationCard& card, base::CardKey key);
    [[nodiscard]] AeroSplineRelaxationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroSplineRelaxationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& splineId1Column() noexcept { return m_splineId1; }

private:
    Column<base::EntityId> m_splineId1;
    CardRows<AeroSplineRelaxationCard> m_cards;
};

} // namespace oor::model
