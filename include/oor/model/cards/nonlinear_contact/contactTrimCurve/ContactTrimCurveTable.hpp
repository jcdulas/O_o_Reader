#pragma once

#include "oor/model/cards/nonlinear_contact/contactTrimCurve/ContactTrimCurveCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactTrimCurveFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactTrimCurveCard& card);
    [[nodiscard]] static std::optional<ContactTrimCurveCard> read(FieldReader& reader);
    static void write(const ContactTrimCurveCard& card, FieldWriter& writer);
};

// All the BCTRIM cards. The IDtrim is unique among them (IdSpace::PerCardType, category TrimmingCurve); a BCNURBS names
// it. The cards are kept whole (CardRows); the query engine reads the ID only.
class ContactTrimCurveTable final : public TypedTable<ContactTrimCurveTable, ContactTrimCurveCard>
{
public:
    static constexpr std::string_view NAME = "ContactTrimCurve";
    static inline const CardTypeInfo INFO{NAME, CardCategory::TrimmingCurve, base::IdSpace::PerCardType};
    using Format = ContactTrimCurveFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactTrimCurveCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactTrimCurveCard& card, base::CardKey key);
    [[nodiscard]] ContactTrimCurveCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactTrimCurveCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no reference

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactTrimCurveCard> m_cards;
};

} // namespace oor::model
