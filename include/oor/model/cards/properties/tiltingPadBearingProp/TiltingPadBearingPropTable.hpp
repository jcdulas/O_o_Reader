#pragma once

#include "oor/model/cards/properties/tiltingPadBearingProp/TiltingPadBearingPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TiltingPadBearingPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TiltingPadBearingPropCard& card);
    [[nodiscard]] static std::optional<TiltingPadBearingPropCard> read(FieldReader& reader);
    static void write(const TiltingPadBearingPropCard& card, FieldWriter& writer);
};

// All the THPAD cards. A THPAD extends the PBUSH2D that has the same number, as an ELEMUDS does: the RID is a reference
// to a property (a renumbering of the properties follows into it) and the card has an ID space of its own
// (IdSpace::PerCardType), in the category Property. The cards are kept whole next to a column of their RIDs.
class TiltingPadBearingPropTable final : public TypedTable<TiltingPadBearingPropTable, TiltingPadBearingPropCard>
{
public:
    static constexpr std::string_view NAME = "TiltingPadBearingProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = TiltingPadBearingPropFormat;

    [[nodiscard]] static base::EntityId cardId(const TiltingPadBearingPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const TiltingPadBearingPropCard& card, base::CardKey key);
    [[nodiscard]] TiltingPadBearingPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TiltingPadBearingPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TiltingPadBearingPropCard> m_cards;
};

} // namespace oor::model
