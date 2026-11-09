#pragma once

#include "oor/model/cards/nonlinear_contact/contactNurbsCurve/ContactNurbsCurveCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactNurbsCurveFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactNurbsCurveCard& card);
    [[nodiscard]] static std::optional<ContactNurbsCurveCard> read(FieldReader& reader);
    static void write(const ContactNurbsCurveCard& card, FieldWriter& writer);
};

// All the BCNURB2 cards (category ContactSurface, unique among them). The cards are kept whole (CardRows); the query
// engine reads the ID only. The grid points are references.
class ContactNurbsCurveTable final : public TypedTable<ContactNurbsCurveTable, ContactNurbsCurveCard>
{
public:
    static constexpr std::string_view NAME = "ContactNurbsCurve";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactNurbsCurveFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactNurbsCurveCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactNurbsCurveCard& card, base::CardKey key);
    [[nodiscard]] ContactNurbsCurveCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactNurbsCurveCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactNurbsCurveCard> m_cards;
};

} // namespace oor::model
