#pragma once

#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactSetCard& card);
    [[nodiscard]] static std::optional<ContactSetCard> read(FieldReader& reader);
    static void write(const ContactSetCard& card, FieldWriter& writer);
};

// All the BCTSET cards. The CSID is a contact set number, unique among the BCTSET cards (IdSpace::PerCardType, category
// ContactTable: the BCSET and BCONTACT commands select a set or a BCTADD, a BCTABL1, a BCTABLE or an MDBCTB1 by the
// same kind of number, which the contact table check of the validation compares across the types). A BCTADD names the
// sets and a BCTPARM or BCTPARA gives one parameters. The regions are not looked up (see ContactSetCard). The cards are
// kept whole next to their ID.
class ContactSetTable final : public TypedTable<ContactSetTable, ContactSetCard>
{
public:
    static constexpr std::string_view NAME = "ContactSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactTable, base::IdSpace::PerCardType};
    using Format = ContactSetFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactSetCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactSetCard& card, base::CardKey key);
    [[nodiscard]] ContactSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ContactSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {
    } // the regions may be BSURFS cards, which are not typed

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ContactSetCard> m_cards;
};

} // namespace oor::model
